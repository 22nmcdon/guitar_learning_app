"""Draws the app icon: a piece of fretboard, which is what the app is.

Pure zlib + struct rather than a library, because adding an image dependency to
a repository that has none - and whose whole build is "cmake, and a shell script
for the wasm" - is a poor trade for two PNGs that change never.
"""
import struct, zlib

ROSEWOOD = (75, 50, 39)
ROSEWOOD_TOP = (93, 64, 51)
STRING = (227, 217, 198)
FRET = (207, 198, 186)
ROOT = (192, 86, 74)
BONE = (239, 230, 214)

def draw(size):
    px = [[ROSEWOOD_TOP if y < size * 0.45 else ROSEWOOD for _ in range(size)] for y in range(size)]
    inset = int(size * 0.11)          # maskable safe zone
    board = size - 2 * inset

    # the nut, on the left
    nut = inset + int(board * 0.10)
    for y in range(inset, size - inset):
        for x in range(nut, nut + max(2, size // 40)):
            px[y][x] = BONE

    # three frets
    for n in (1, 2, 3):
        x0 = nut + int(board * 0.26 * n)
        for y in range(inset, size - inset):
            for x in range(x0, x0 + max(1, size // 96)):
                if x < size - inset:
                    px[y][x] = FRET

    # six strings, thicker at the bottom
    for s in range(6):
        y0 = inset + int(board * (s + 0.5) / 6)
        gauge = max(1, int(size * (0.012 - 0.0014 * s)))
        for y in range(y0, y0 + gauge):
            for x in range(inset, size - inset):
                if 0 <= y < size:
                    px[y][x] = STRING

    # one fingered note, where a root would sit
    cx = nut + int(board * 0.39)
    cy = inset + int(board * 4.5 / 6)
    r = int(size * 0.085)
    for y in range(max(0, cy - r), min(size, cy + r)):
        for x in range(max(0, cx - r), min(size, cx + r)):
            if (x - cx) ** 2 + (y - cy) ** 2 <= r * r:
                px[y][x] = ROOT

    return px

def png(path, size):
    raw = b"".join(b"\x00" + bytes(v for p in row for v in p) for row in draw(size))

    def chunk(tag, data):
        body = tag + data
        return struct.pack(">I", len(data)) + body + struct.pack(">I", zlib.crc32(body) & 0xffffffff)

    with open(path, "wb") as f:
        f.write(b"\x89PNG\r\n\x1a\n")
        f.write(chunk(b"IHDR", struct.pack(">IIBBBBB", size, size, 8, 2, 0, 0, 0)))
        f.write(chunk(b"IDAT", zlib.compress(raw, 9)))
        f.write(chunk(b"IEND", b""))

png("web/icon-192.png", 192)
png("web/icon-512.png", 512)
print("drew web/icon-192.png and web/icon-512.png")
