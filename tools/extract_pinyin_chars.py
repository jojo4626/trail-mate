#!/usr/bin/env python3

from __future__ import annotations

import argparse
from pathlib import Path


DEFAULT_SOURCE = Path("modules/ui_shared/include/ui/widgets/ime/pinyin_data.h")
DEFAULT_SORTED_OUTPUT = Path("tools/pinyin_chars.txt")
DEFAULT_RANKED_OUTPUT = Path("tools/pinyin_ranked_chars.txt")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Extract unique and ranked Chinese glyph sets from the built-in Pinyin IME dictionary."
    )
    parser.add_argument(
        "--source",
        type=Path,
        default=DEFAULT_SOURCE,
        help="Path to pinyin_data.h",
    )
    parser.add_argument(
        "--sorted-output",
        type=Path,
        default=DEFAULT_SORTED_OUTPUT,
        help="Output path for the codepoint-sorted unique glyph file.",
    )
    parser.add_argument(
        "--ranked-output",
        type=Path,
        default=DEFAULT_RANKED_OUTPUT,
        help="Output path for the frequency-ranked glyph file.",
    )
    return parser.parse_args()


def iter_candidate_lines(source_text: str):
    in_dict = False
    for raw_line in source_text.splitlines():
        line = raw_line.rstrip("\r")
        if not in_dict:
            marker = 'R"PINYIN_DICT('
            if marker not in line:
                continue
            line = line.split(marker, 1)[1]
            in_dict = True

        if ')PINYIN_DICT"' in line:
            line = line.split(')PINYIN_DICT"', 1)[0]
            if line:
                yield line
            break

        yield line


def is_cjk_candidate_char(ch: str) -> bool:
    return len(ch) == 1 and not ch.isspace() and ord(ch) >= 0x80


def wrap_chars(chars: list[str], width: int = 64) -> str:
    if not chars:
        return ""
    return "\n".join("".join(chars[index:index + width]) for index in range(0, len(chars), width)) + "\n"


def main() -> int:
    args = parse_args()

    source_path = args.source.resolve()
    if not source_path.exists():
        raise FileNotFoundError(f"pinyin dictionary source not found: {source_path}")

    source_text = source_path.read_text(encoding="utf-8")

    unique_chars: set[str] = set()
    ranked_scores: dict[str, int] = {}
    first_seen_order: dict[str, int] = {}
    next_seen_index = 0

    for line in iter_candidate_lines(source_text):
        if not line or line.startswith("#"):
            continue
        if "\t" not in line:
            continue

        _, candidate_text = line.split("\t", 1)
        tokens = [token.strip() for token in candidate_text.split(" ") if token.strip()]
        for token_index, token in enumerate(tokens):
            token_weight = max(1, 256 - token_index)
            for ch in token:
                if not is_cjk_candidate_char(ch):
                    continue
                unique_chars.add(ch)
                ranked_scores[ch] = ranked_scores.get(ch, 0) + token_weight
                if ch not in first_seen_order:
                    first_seen_order[ch] = next_seen_index
                    next_seen_index += 1

    sorted_chars = sorted(unique_chars, key=ord)
    ranked_chars = sorted(
        unique_chars,
        key=lambda ch: (-ranked_scores.get(ch, 0), first_seen_order.get(ch, 0), ord(ch)),
    )

    args.sorted_output.parent.mkdir(parents=True, exist_ok=True)
    args.sorted_output.write_text(wrap_chars(sorted_chars), encoding="utf-8")

    args.ranked_output.parent.mkdir(parents=True, exist_ok=True)
    args.ranked_output.write_text(wrap_chars(ranked_chars), encoding="utf-8")

    print(f"source={source_path}")
    print(f"unique_glyph_count={len(sorted_chars)}")
    print(f"sorted_output={args.sorted_output.resolve()}")
    print(f"ranked_output={args.ranked_output.resolve()}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
