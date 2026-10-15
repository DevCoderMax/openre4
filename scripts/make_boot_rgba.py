#!/usr/bin/env python3
"""Converte assets/boot.png em RGBA8 cru + .meta (w/h) para o splash de boot.

Uso: python3 scripts/make_boot_rgba.py [saida_dir]
Gera: <saida_dir>/boot.rgba (w*h*4 bytes) e <saida_dir>/boot.meta ("w h").
"""
import os
import struct
import sys
import zlib

SRC = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "assets", "boot.png")


def _read_png_rgba(path):
    """Le PNG 8-bit nao-entrelaçado (RGB/RGBA/gray) só com a stdlib."""
    with open(path, "rb") as f:
        sig = f.read(8)
    if sig != b"\x89PNG\r\n\x1a\n":
        raise ValueError("nao e PNG")
    data = open(path, "rb").read()
    pos = 8
    w = h = None
    bitd = ctyp = None
    raw = b""
    while pos < len(data):
        (ln,) = struct.unpack(">I", data[pos:pos + 4])
        typ = data[pos + 4:pos + 8]
        chunk = data[pos + 8:pos + 8 + ln]
        if typ == b"IHDR":
            w, h, bitd, ctyp, comp, filt, inter = struct.unpack(">IIBBBBB", chunk)
            if bitd != 8 or inter != 0:
                raise ValueError("PNG suportado: 8-bit nao-entrelaçado")
        elif typ == b"IDAT":
            raw += chunk
        elif typ == b"IEND":
            break
        pos += 12 + ln
    px = zlib.decompress(raw)
    ch = {0: 1, 2: 3, 4: 2, 6: 4}[ctyp]
    stride = w * ch
    out = bytearray(w * h * 4)
    prev = bytearray(stride)
    p = 0
    for y in range(h):
        f = px[p]
        p += 1
        cur = bytearray(px[p:p + stride])
        p += stride
        if f == 1:
            for i in range(ch, stride):
                cur[i] = (cur[i] + cur[i - ch]) & 0xFF
        elif f == 2:
            for i in range(stride):
                cur[i] = (cur[i] + prev[i]) & 0xFF
        elif f == 3:
            for i in range(stride):
                a = cur[i - ch] if i >= ch else 0
                cur[i] = (cur[i] + ((a + prev[i]) >> 1)) & 0xFF
        elif f == 4:
            for i in range(stride):
                a = cur[i - ch] if i >= ch else 0
                b = prev[i]
                c = prev[i - ch] if i >= ch else 0
                pa, pb, pc = abs(b - c), abs(a - c), abs(a + b - 2 * c)
                pr = a if (pa <= pb and pa <= pc) else (b if pb <= pc else c)
                cur[i] = (cur[i] + pr) & 0xFF
        o = y * w * 4
        if ch == 4:
            out[o:o + w * 4] = cur
        elif ch == 3:
            for i in range(w):
                out[o + i * 4:o + i * 4 + 4] = bytes((cur[i * 3], cur[i * 3 + 1], cur[i * 3 + 2], 255))
        elif ch == 2:
            for i in range(w):
                g, a = cur[i * 2], cur[i * 2 + 1]
                out[o + i * 4:o + i * 4 + 4] = bytes((g, g, g, a))
        else:
            for i in range(w):
                g = cur[i]
                out[o + i * 4:o + i * 4 + 4] = bytes((g, g, g, 255))
        prev = cur
    return w, h, bytes(out)


def read_rgba(path):
    try:
        from PIL import Image

        img = Image.open(path).convert("RGBA")
        return img.size[0], img.size[1], img.tobytes()
    except ImportError:
        return _read_png_rgba(path)


def main():
    out_dir = sys.argv[1] if len(sys.argv) > 1 else "build/boot_assets"
    os.makedirs(out_dir, exist_ok=True)
    img_w, img_h, rgba = read_rgba(SRC)
    with open(os.path.join(out_dir, "boot.rgba"), "wb") as f:
        f.write(rgba)
    with open(os.path.join(out_dir, "boot.meta"), "w") as f:
        f.write("%d %d\n" % (img_w, img_h))
    print("boot splash: %dx%d -> %s/" % (img_w, img_h, out_dir))


if __name__ == "__main__":
    main()
