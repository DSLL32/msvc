#!/usr/bin/env python3
"""Create small, deterministic resource inputs; no executable data is copied."""
from pathlib import Path
import struct

HERE = Path(__file__).resolve().parent
OUT = HERE / 'source/assets'


def dib(size):
    stride = ((size + 31) // 32) * 4
    rows = []
    for y in range(size):
        row = bytearray(stride)
        for x in range(size):
            if x == y or x == size - 1 - y or x in (0, size - 1) or y in (0, size - 1):
                row[x // 8] |= 0x80 >> (x % 8)
        rows.append(bytes(row))
    pixels = b''.join(rows)
    return (struct.pack('<IiiHHIIiiII', 40, size, size * 2, 1, 1, 0,
                        len(pixels) * 2, 0, 0, 2, 0)
            + b'\x00\x00\x00\x00\xff\xff\xff\x00'
            + pixels + b'\0' * len(pixels))


def main():
    OUT.mkdir(parents=True, exist_ok=True)
    images = [dib(16), dib(32)]
    offset = 6 + 16 * len(images)
    entries = []
    for size, data in zip((16, 32), images):
        entries.append(struct.pack('<BBBBHHII', size, size, 2, 0, 1, 1, len(data), offset))
        offset += len(data)
    (OUT / 'multi.ico').write_bytes(struct.pack('<HHH', 0, 1, 2) + b''.join(entries) + b''.join(images))
    image = dib(16)
    (OUT / 'pointer.cur').write_bytes(struct.pack('<HHH', 0, 2, 1)
        + struct.pack('<BBBBHHII', 16, 16, 2, 0, 1, 1, len(image), 22) + image)
    pixels = b''.join(bytes((x * 31, y * 31, (x ^ y) * 31)) for y in range(8) for x in range(8))
    (OUT / 'pattern.bmp').write_bytes(struct.pack('<2sIHHI', b'BM', 54 + len(pixels), 0, 0, 54)
        + struct.pack('<IiiHHIIiiII', 40, 8, 8, 1, 24, 0, len(pixels), 0, 0, 0, 0) + pixels)
    (OUT / 'payload.bin').write_bytes(b'Fixture payload\0\r\n' + bytes(range(32)))
    text = b'Resource message %1!s!\r\n\0'
    length = (4 + len(text) + 3) & ~3
    message = struct.pack('<HH', length, 0) + text
    message += b'\0' * (length - len(message))
    (OUT / 'messages.bin').write_bytes(struct.pack('<IIII', 1, 0x40010001, 0x40010001, 16) + message)


if __name__ == '__main__':
    main()
