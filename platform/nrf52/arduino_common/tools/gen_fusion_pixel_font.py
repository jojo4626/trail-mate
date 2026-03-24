#!/usr/bin/env python3
from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path
import argparse
import bisect


FONT_ASCENT = 7
FONT_DESCENT = 1
CELL_WIDTH = 8
CELL_HEIGHT = 8


@dataclass
class Glyph:
    codepoint: int
    advance: int
    bitmap: bytes


def decode_gb2312_range(high_start: int, high_end_inclusive: int) -> list[int]:
    chars: list[int] = []
    for high in range(high_start, high_end_inclusive + 1):
        for low in range(0xA1, 0xFF):
            try:
                ch = bytes((high, low)).decode("gb2312")
            except UnicodeDecodeError:
                continue
            chars.append(ord(ch))
    return chars


def decode_gb2312_level1() -> list[int]:
    return decode_gb2312_range(0xB0, 0xD7)


def decode_gb2312_level2() -> list[int]:
    return decode_gb2312_range(0xD8, 0xF7)


def default_charset() -> list[int]:
    codepoints = set(range(0x20, 0x7F))
    codepoints.update(decode_gb2312_level1())
    codepoints.update(decode_gb2312_level2())
    codepoints.update(range(0x2500, 0x2580))
    codepoints.update(range(0x2580, 0x25A0))
    codepoints.update(range(0x25A0, 0x2600))
    codepoints.update({
        0x00B0,
        0x2190,
        0x2191,
        0x2192,
        0x2193,
        0x21E4,
        0x21E6,
        0x21EA,
        0x21F0,
        0x25CB,
        0x25CF,
        0x2605,
        0x3001,
        0x3002,
        0xFF1A,
        0xFF01,
        0xFF08,
        0xFF09,
        0x2014,
        0x2026,
    })
    return sorted(codepoints)


def parse_bdf(path: Path) -> tuple[dict[int, Glyph], int]:
    glyphs: dict[int, Glyph] = {}
    default_char = ord("?")
    lines = path.read_text(encoding="utf-8", errors="ignore").splitlines()
    i = 0
    while i < len(lines):
        line = lines[i]
        if line.startswith("DEFAULT_CHAR "):
            default_char = int(line.split()[1])
        if line != "STARTCHAR" and not line.startswith("STARTCHAR "):
            i += 1
            continue

        codepoint = None
        advance = CELL_WIDTH
        bbw = CELL_WIDTH
        bbh = CELL_HEIGHT
        bbx = 0
        bby = -1
        bitmap_rows: list[tuple[int, int]] = []
        i += 1
        while i < len(lines):
            line = lines[i]
            if line.startswith("ENCODING "):
                codepoint = int(line.split()[1])
            elif line.startswith("DWIDTH "):
                advance = int(line.split()[1])
            elif line.startswith("BBX "):
                _, w, h, x, y = line.split()
                bbw, bbh, bbx, bby = int(w), int(h), int(x), int(y)
            elif line == "BITMAP":
                i += 1
                while i < len(lines) and lines[i] != "ENDCHAR":
                    row_text = lines[i].strip()
                    bitmap_rows.append((int(row_text, 16), len(row_text) * 4))
                    i += 1
                break
            i += 1

        if codepoint is None:
            continue

        cell = [0] * CELL_HEIGHT
        top = FONT_ASCENT - (bbh + bby)
        for row_idx in range(min(bbh, len(bitmap_rows))):
            cell_y = top + row_idx
            if not (0 <= cell_y < CELL_HEIGHT):
                continue
            row_value, row_bits = bitmap_rows[row_idx]
            dest = 0
            for col in range(bbw):
                if col >= row_bits:
                    continue
                bit = (row_value >> (row_bits - 1 - col)) & 1
                cell_x = bbx + col
                if 0 <= cell_x < CELL_WIDTH and bit:
                    dest |= 1 << (7 - cell_x)
            cell[cell_y] = dest

        glyphs[codepoint] = Glyph(
            codepoint=codepoint,
            advance=max(1, min(advance, CELL_WIDTH)),
            bitmap=bytes(cell),
        )
        i += 1
    return glyphs, default_char


def write_header(path: Path) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(
        "#pragma once\n\n"
        "#include \"ui/mono_128x64/font/mono_font.h\"\n\n"
        "namespace ui::mono_128x64\n{\n"
        "// Generated from Fusion Pixel 8px BDF for the NRF mono UI path.\n"
        "// Glyph bitmaps are normalized into fixed 8x8 cells so the renderer\n"
        "// can keep a single compact raster contract for ASCII and CJK.\n"
        "extern const MonoFont kFusionPixel8Font;\n"
        "} // namespace ui::mono_128x64\n",
        encoding="utf-8",
    )


def format_bytes(data: bytes, per_line: int = 16) -> str:
    toks = [f"0x{b:02X}" for b in data]
    lines = []
    for i in range(0, len(toks), per_line):
        lines.append("    " + ", ".join(toks[i : i + per_line]) + ",")
    return "\n".join(lines)


def format_bitmap(entries: list[Glyph]) -> tuple[str, str, str]:
    bitmap = bytearray()
    cp_lines = []
    advance_lines = []
    for g in entries:
        bitmap.extend(g.bitmap)
        cp_lines.append(f"    0x{g.codepoint:04X},")
        advance_lines.append(f"    0x{g.advance:02X},")
    return format_bytes(bytes(bitmap)), "\n".join(cp_lines), "\n".join(advance_lines)


def write_source(path: Path, entries: list[Glyph], fallback_index: int) -> None:
    bitmap_body, cp_body, advance_body = format_bitmap(entries)
    path.parent.mkdir(parents=True, exist_ok=True)
    text = f"""#include "ui/fonts/fusion_pixel_8_font_generated.h"

namespace ui::mono_128x64
{{

static const uint8_t kFusionPixel8Bitmap[] = {{
{bitmap_body}
}};

static const uint16_t kFusionPixel8Codepoints[] = {{
{cp_body}
}};

static const uint8_t kFusionPixel8Advances[] = {{
{advance_body}
}};

const MonoFont kFusionPixel8Font = MonoFont::makeCompact16(
    kFusionPixel8Bitmap,
    kFusionPixel8Codepoints,
    kFusionPixel8Advances,
    static_cast<uint16_t>({len(entries)}),
    {CELL_HEIGHT},
    {FONT_ASCENT},
    {CELL_WIDTH},
    static_cast<uint16_t>({fallback_index}),
    {CELL_WIDTH},
    {CELL_HEIGHT},
    {CELL_WIDTH});

}} // namespace ui::mono_128x64
"""
    path.write_text(text, encoding="utf-8")


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("--bdf", required=True)
    ap.add_argument("--out-header", required=True)
    ap.add_argument("--out-source", required=True)
    args = ap.parse_args()

    glyph_map, default_char = parse_bdf(Path(args.bdf))
    wanted = default_charset()
    if default_char not in glyph_map and ord("?") in glyph_map:
        default_char = ord("?")

    selected: list[Glyph] = []
    selected_codes: list[int] = []
    for cp in wanted:
        g = glyph_map.get(cp)
        if g is None:
            continue
        selected.append(g)
        selected_codes.append(cp)

    if default_char not in selected_codes and default_char in glyph_map:
        selected.append(glyph_map[default_char])
        selected_codes.append(default_char)

    order = sorted(range(len(selected_codes)), key=lambda idx: selected_codes[idx])
    selected = [selected[idx] for idx in order]
    selected_codes = [selected_codes[idx] for idx in order]

    fallback_index = bisect.bisect_left(selected_codes, default_char)
    if fallback_index >= len(selected_codes) or selected_codes[fallback_index] != default_char:
        fallback_index = bisect.bisect_left(selected_codes, ord("?"))

    write_header(Path(args.out_header))
    write_source(Path(args.out_source), selected, fallback_index)
    print(f"generated glyphs={len(selected)} fallback=U+{selected_codes[fallback_index]:04X}")


if __name__ == "__main__":
    main()
