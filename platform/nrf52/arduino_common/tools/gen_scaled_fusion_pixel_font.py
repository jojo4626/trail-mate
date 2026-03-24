#!/usr/bin/env python3
from __future__ import annotations

import argparse
import math
import re
from pathlib import Path


ARRAY_RE = re.compile(
    r"static const uint(8|16)_t (kFusionPixel8(?:Bitmap|Codepoints|Advances))\[\] = \{\s*(.*?)\s*\};",
    re.S,
)
COUNT_RE = re.compile(r"static_cast<uint16_t>\((\d+)\)")


def parse_array(body: str) -> list[int]:
    return [int(token, 0) for token in re.findall(r"0x[0-9A-Fa-f]+|\d+", body)]


def parse_source(path: Path) -> tuple[list[int], list[int], list[int], int]:
    text = path.read_text(encoding="utf-8")
    found: dict[str, list[int]] = {}
    for _bits, name, body in ARRAY_RE.findall(text):
        found[name] = parse_array(body)

    counts = [int(v) for v in COUNT_RE.findall(text)]
    if len(counts) < 2:
        raise RuntimeError("unable to parse glyph count/fallback index from source")

    glyph_count = counts[0]
    bitmap = found["kFusionPixel8Bitmap"]
    codepoints = found["kFusionPixel8Codepoints"]
    advances = found["kFusionPixel8Advances"]
    return bitmap, codepoints, advances, glyph_count


def ascii_charset() -> list[int]:
    return list(range(0x20, 0x7F))


def filter_charset(bitmap: list[int],
                   codepoints: list[int],
                   advances: list[int],
                   glyph_count: int,
                   allowed_codepoints: set[int]) -> tuple[list[int], list[int], list[int], int]:
    filtered_bitmap: list[int] = []
    filtered_codepoints: list[int] = []
    filtered_advances: list[int] = []

    for glyph_index in range(glyph_count):
        codepoint = codepoints[glyph_index]
        if codepoint not in allowed_codepoints:
            continue
        start = glyph_index * 8
        filtered_bitmap.extend(bitmap[start:start + 8])
        filtered_codepoints.append(codepoint)
        filtered_advances.append(advances[glyph_index])

    return filtered_bitmap, filtered_codepoints, filtered_advances, len(filtered_codepoints)


def upscale_glyph(rows8: list[int], dest_size: int = 10) -> list[int]:
    rows10: list[int] = []
    for y in range(dest_size):
        src_y = min(7, (y * 8) // dest_size)
        src_row = rows8[src_y]
        dest_row = 0
        for x in range(dest_size):
            src_x = min(7, (x * 8) // dest_size)
            bit = (src_row >> (7 - src_x)) & 0x01
            if bit:
                dest_row |= 1 << (15 - x)
        rows10.append(dest_row)
    return rows10


def scale_advance(adv: int, dest_size: int = 10) -> int:
    return max(1, min(dest_size, int(math.floor((adv * dest_size / 8.0) + 0.5))))


def format_bytes(data: list[int], per_line: int = 16) -> str:
    toks = [f"0x{value:02X}" for value in data]
    lines = []
    for idx in range(0, len(toks), per_line):
        lines.append("    " + ", ".join(toks[idx:idx + per_line]) + ",")
    return "\n".join(lines)


def format_words(data: list[int], per_line: int = 8) -> str:
    toks = [f"0x{value:04X}" for value in data]
    lines = []
    for idx in range(0, len(toks), per_line):
        lines.append("    " + ", ".join(toks[idx:idx + per_line]) + ",")
    return "\n".join(lines)


def write_source(path: Path, bitmap8: list[int], codepoints: list[int], advances8: list[int], glyph_count: int) -> None:
    bitmap10: list[int] = []
    advances10 = [scale_advance(value) for value in advances8]

    for glyph_index in range(glyph_count):
        rows8 = bitmap8[glyph_index * 8:(glyph_index + 1) * 8]
        rows10 = upscale_glyph(rows8)
        for row in rows10:
            bitmap10.append((row >> 8) & 0xFF)
            bitmap10.append(row & 0xFF)

    fallback_index = codepoints.index(ord("?")) if ord("?") in codepoints else 0

    text = f"""#ifndef TRAIL_MATE_NRF_MONO_FUSION_PIXEL_10_ENABLED
#define TRAIL_MATE_NRF_MONO_FUSION_PIXEL_10_ENABLED 0
#endif

#if TRAIL_MATE_NRF_MONO_FUSION_PIXEL_10_ENABLED

#include "ui/fonts/fusion_pixel_10_font_generated.h"

namespace ui::mono_128x64
{{

static const uint8_t kFusionPixel10Bitmap[] = {{
{format_bytes(bitmap10)}
}};

static const uint16_t kFusionPixel10Codepoints[] = {{
{format_words(codepoints)}
}};

static const uint8_t kFusionPixel10Advances[] = {{
{format_bytes(advances10)}
}};

const MonoFont kFusionPixel10Font = MonoFont::makeCompact16(
    kFusionPixel10Bitmap,
    kFusionPixel10Codepoints,
    kFusionPixel10Advances,
    static_cast<uint16_t>({glyph_count}),
    10,
    9,
    10,
    static_cast<uint16_t>({fallback_index}),
    10,
    10,
    10);

}} // namespace ui::mono_128x64

#endif
"""
    path.write_text(text, encoding="utf-8")


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("--source8", required=True)
    ap.add_argument("--out-source", required=True)
    ap.add_argument("--ascii-only", action="store_true")
    args = ap.parse_args()

    bitmap, codepoints, advances, glyph_count = parse_source(Path(args.source8))
    if args.ascii_only:
        bitmap, codepoints, advances, glyph_count = filter_charset(bitmap,
                                                                   codepoints,
                                                                   advances,
                                                                   glyph_count,
                                                                   set(ascii_charset()))
        if ord("?") not in codepoints:
            raise RuntimeError("ASCII subset must include '?' fallback glyph")
    write_source(Path(args.out_source), bitmap, codepoints, advances, glyph_count)


if __name__ == "__main__":
    main()
