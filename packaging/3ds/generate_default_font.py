#!/usr/bin/env python3

import re
import struct
import sys
import zlib
from pathlib import Path


def png_chunk(kind: bytes, data: bytes) -> bytes:
    payload = kind + data
    return struct.pack(">I", len(data)) + payload + struct.pack(">I", zlib.crc32(payload) & 0xFFFFFFFF)


def main() -> int:
    if len(sys.argv) != 3:
        print("usage: generate_default_font.py <raylib-rtext.c> <output.png>", file=sys.stderr)
        return 2

    source = Path(sys.argv[1]).read_text(encoding="utf-8")
    match = re.search(r"unsigned int defaultFontData\[512\]\s*=\s*\{(.*?)\};", source, re.DOTALL)
    if not match:
        print("could not find raylib defaultFontData", file=sys.stderr)
        return 1

    words = [int(value, 16) for value in re.findall(r"0x[0-9a-fA-F]+", match.group(1))]
    if len(words) != 512:
        print(f"expected 512 font words, found {len(words)}", file=sys.stderr)
        return 1

    width = 128
    height = 128
    rows = bytearray()
    for y in range(height):
        rows.append(0)  # PNG filter: none
        for x in range(width):
            pixel = y * width + x
            visible = (words[pixel // 32] >> (pixel % 32)) & 1
            rows.extend((255, 255, 255, 255 if visible else 0))

    png = b"\x89PNG\r\n\x1a\n"
    png += png_chunk(b"IHDR", struct.pack(">IIBBBBB", width, height, 8, 6, 0, 0, 0))
    png += png_chunk(b"IDAT", zlib.compress(bytes(rows), 9))
    png += png_chunk(b"IEND", b"")

    output = Path(sys.argv[2])
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_bytes(png)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
