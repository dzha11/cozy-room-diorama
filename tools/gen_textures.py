#!/usr/bin/env python3
"""Generates every texture in assets/textures procedurally.

All textures in this project are original and made by this script, so there
are no third-party texture licences to track. Run from the repository root:

    python3 tools/gen_textures.py

Requires numpy and Pillow (pip install numpy pillow).
"""
import math
import os

import numpy as np
from PIL import Image, ImageDraw, ImageFilter

OUT = os.path.join(os.path.dirname(__file__), "..", "assets", "textures")
rng = np.random.default_rng(7)


def save(img, name):
    os.makedirs(OUT, exist_ok=True)
    path = os.path.join(OUT, name)
    img.save(path, optimize=True)
    print("wrote", os.path.normpath(path))


def tile_noise(size, cells, seed):
    """Smooth value noise that tiles seamlessly (bicubic upscaled random grid)."""
    r = np.random.default_rng(seed)
    grid = r.random((cells, cells)).astype(np.float32)
    big = np.tile(grid, (3, 3))
    img = Image.fromarray((big * 255).astype(np.uint8)).resize((size * 3, size * 3), Image.BICUBIC)
    a = np.asarray(img, dtype=np.float32)[size:2 * size, size:2 * size] / 255.0
    return a


def fbm(size, seed, octaves=(4, 8, 16, 32, 64)):
    out = np.zeros((size, size), np.float32)
    amp, total = 1.0, 0.0
    for i, c in enumerate(octaves):
        out += tile_noise(size, c, seed + i) * amp
        total += amp
        amp *= 0.5
    return out / total


def to_img(rgb):
    return Image.fromarray(np.clip(rgb * 255, 0, 255).astype(np.uint8), "RGB")


# --------------------------------------------------------------------------
def floor_wood(size=1024, planks=6):
    """Oak floor planks running along X, with staggered joints and grain."""
    y, x = np.mgrid[0:size, 0:size].astype(np.float32) / size
    rgb = np.zeros((size, size, 3), np.float32)
    grain = fbm(size, 11, (2, 4, 64, 128))
    fine = tile_noise(size, 256, 99)
    base_cols = np.array([[0.62, 0.43, 0.27], [0.59, 0.41, 0.25], [0.64, 0.45, 0.28],
                          [0.58, 0.40, 0.24], [0.61, 0.42, 0.26], [0.63, 0.44, 0.27]])
    row = np.floor(y * planks).astype(int)
    v = (y * planks) % 1.0
    # each row is split into two boards with a joint at a random offset
    offsets = rng.random(planks)
    seg = np.floor((x + offsets[row]) * 2) % 2
    board_id = (row * 2 + seg.astype(int)) % len(base_cols)
    col = base_cols[board_id]
    stripes = 0.5 + 0.5 * np.sin((v * 9 + grain * 6 + seg * 3) * math.pi)
    shade = 0.88 + 0.10 * stripes + 0.08 * (fine - 0.5)
    rgb = col * shade[..., None]
    # dark seams between planks and at board ends
    seam = (v < 0.012) | (v > 0.988)
    joint = np.abs(((x + offsets[row]) * 2) % 1.0) < 0.004
    rgb[seam | joint] *= 0.35
    save(to_img(rgb), "floor_wood.png")


def wallpaper(size=512):
    """Warm cream wallpaper with soft vertical stripes and small dots."""
    y, x = np.mgrid[0:size, 0:size].astype(np.float32) / size
    base = np.array([0.87, 0.80, 0.70])
    stripe = 0.5 + 0.5 * np.cos(x * 8 * 2 * math.pi)
    rgb = base * (0.95 + 0.05 * stripe)[..., None]
    paper = fbm(size, 21, (32, 64, 128))
    rgb *= (0.95 + 0.08 * paper)[..., None]
    img = to_img(rgb)
    d = ImageDraw.Draw(img)
    n = 8
    for i in range(n):
        for j in range(n):
            cx = (i + 0.5 + (0.5 if j % 2 else 0)) * size / n
            cy = (j + 0.5) * size / n
            for ox in (-size, 0, size):  # wrap so the tile is seamless
                d.ellipse([cx + ox - 5, cy - 5, cx + ox + 5, cy + 5], fill=(196, 150, 120))
    save(img.filter(ImageFilter.GaussianBlur(0.7)), "wallpaper.png")


def wood(size=512):
    """Generic light wood grain, tinted per object by the material colour."""
    y, x = np.mgrid[0:size, 0:size].astype(np.float32) / size
    grain = fbm(size, 31, (2, 4, 8, 128))
    rings = 0.5 + 0.5 * np.sin((y * 14 + grain * 5) * 2 * math.pi)
    fine = tile_noise(size, 256, 32)
    v = 0.80 + 0.14 * rings + 0.06 * (fine - 0.5)
    rgb = np.stack([v * 0.93, v * 0.76, v * 0.58], -1)
    save(to_img(rgb), "wood.png")


def fabric_plain(size=512):
    """Linen weave, light grey, tinted per object."""
    y, x = np.mgrid[0:size, 0:size].astype(np.float32)
    weave = 0.5 + 0.25 * np.sin(x * 2 * math.pi / 4) + 0.25 * np.sin(y * 2 * math.pi / 4)
    n = fbm(size, 41, (16, 64, 128))
    v = 0.82 + 0.10 * weave + 0.10 * (n - 0.5)
    rgb = np.stack([v, v, v * 0.98], -1)
    save(to_img(rgb), "fabric.png")


def blanket(size=512):
    """Cozy plaid blanket."""
    y, x = np.mgrid[0:size, 0:size].astype(np.float32) / size
    base = np.array([0.72, 0.30, 0.26])
    band_x = (np.cos(x * 4 * 2 * math.pi) > 0.6).astype(np.float32)
    band_y = (np.cos(y * 4 * 2 * math.pi) > 0.6).astype(np.float32)
    thin_x = (np.abs(np.cos(x * 4 * 2 * math.pi + math.pi)) > 0.985).astype(np.float32)
    thin_y = (np.abs(np.cos(y * 4 * 2 * math.pi + math.pi)) > 0.985).astype(np.float32)
    rgb = np.ones((size, size, 3), np.float32) * base
    rgb = rgb * (1 - 0.25 * band_x[..., None]) * (1 - 0.25 * band_y[..., None])
    cream = np.array([0.93, 0.86, 0.72])
    mask = np.maximum(thin_x, thin_y)[..., None]
    rgb = rgb * (1 - mask) + cream * mask
    knit = 0.92 + 0.08 * np.sin(x * size * math.pi / 3) * np.sin(y * size * math.pi / 3)
    rgb *= knit[..., None]
    save(to_img(rgb), "blanket.png")


def rug(w=1024, h=768):
    """Patterned rug with border and diamonds (used once, 0..1 UVs)."""
    img = Image.new("RGB", (w, h), (52, 72, 98))
    d = ImageDraw.Draw(img)
    d.rectangle([30, 30, w - 31, h - 31], outline=(225, 205, 160), width=18)
    d.rectangle([70, 70, w - 71, h - 71], outline=(176, 92, 64), width=10)
    for i in range(7):
        for j in range(5):
            cx = 150 + i * (w - 300) / 6
            cy = 150 + j * (h - 300) / 4
            s = 46
            col = (225, 205, 160) if (i + j) % 2 == 0 else (176, 92, 64)
            d.polygon([(cx, cy - s), (cx + s, cy), (cx, cy + s), (cx - s, cy)], outline=col, fill=None, width=8)
            d.ellipse([cx - 8, cy - 8, cx + 8, cy + 8], fill=col)
    a = np.asarray(img, np.float32) / 255.0
    n = tile_noise(512, 128, 51)
    n = np.asarray(Image.fromarray((n * 255).astype(np.uint8)).resize((w, h)), np.float32) / 255
    a *= (0.88 + 0.16 * n)[..., None]
    save(to_img(a).filter(ImageFilter.GaussianBlur(1.0)), "rug.png")


def poster(w=600, h=840):
    """Original lo-fi landscape: layered mountains under a big sun."""
    img = Image.new("RGB", (w, h))
    d = ImageDraw.Draw(img)
    top, bottom = np.array([44, 36, 92]), np.array([250, 150, 110])
    for yy in range(h):
        t = min(1.0, yy / (h * 0.65))
        d.line([(0, yy), (w, yy)], fill=tuple(int(c) for c in top * (1 - t) + bottom * t))
    d.ellipse([w * 0.28, h * 0.30, w * 0.72, h * 0.30 + w * 0.44], fill=(255, 214, 140))
    layers = [((112, 64, 120), 0.52, 3), ((80, 46, 98), 0.60, 5), ((50, 30, 70), 0.70, 7), ((28, 18, 44), 0.80, 4)]
    for col, base, freq in layers:
        pts = [(0, h)]
        for xx in range(0, w + 1, 6):
            yv = base * h + 40 * math.sin(xx / w * freq * math.pi + base * 10) + 18 * math.sin(xx / w * freq * 5.3)
            pts.append((xx, yv))
        pts.append((w, h))
        d.polygon(pts, fill=col)
    d.rectangle([0, 0, w - 1, h - 1], outline=(240, 236, 228), width=24)
    save(img, "poster.png")


def clock_face(size=512):
    img = Image.new("RGB", (size, size), (243, 236, 222))
    d = ImageDraw.Draw(img)
    c = size / 2
    for i in range(60):
        a = i / 60 * 2 * math.pi
        r0 = size * (0.40 if i % 5 else 0.36)
        r1 = size * 0.45
        width = 10 if i % 5 == 0 else 3
        d.line([(c + r0 * math.sin(a), c - r0 * math.cos(a)), (c + r1 * math.sin(a), c - r1 * math.cos(a))],
               fill=(40, 40, 46), width=width)
    d.ellipse([c - 14, c - 14, c + 14, c + 14], fill=(40, 40, 46))
    save(img, "clock_face.png")


def vinyl(size=512):
    y, x = np.mgrid[0:size, 0:size].astype(np.float32)
    r = np.hypot(x - size / 2, y - size / 2) / (size / 2)
    grooves = 0.08 + 0.04 * np.sin(r * 260)
    rgb = np.stack([grooves] * 3, -1)
    label = r < 0.34
    rgb[label] = [0.86, 0.42, 0.30]
    rgb[(r > 0.30) & (r < 0.32)] = [0.95, 0.85, 0.70]
    rgb[r < 0.03] = [0.05, 0.05, 0.05]
    save(to_img(rgb), "vinyl.png")


def screen_code(w=512, h=320):
    """Laptop screen: dark editor with coloured 'code' lines (emissive)."""
    img = Image.new("RGB", (w, h), (24, 26, 36))
    d = ImageDraw.Draw(img)
    d.rectangle([0, 0, 70, h], fill=(32, 34, 46))
    palette = [(198, 120, 221), (97, 175, 239), (152, 195, 121), (229, 192, 123), (224, 108, 117), (171, 178, 191)]
    r = np.random.default_rng(3)
    yy = 14
    while yy < h - 12:
        xx = 90 + int(r.integers(0, 4)) * 22
        for _ in range(int(r.integers(1, 5))):
            ln = int(r.integers(20, 90))
            d.rounded_rectangle([xx, yy, xx + ln, yy + 7], radius=3, fill=palette[int(r.integers(0, 6))])
            xx += ln + 10
            if xx > w - 40:
                break
        d.rectangle([20, yy, 50, yy + 7], fill=(80, 84, 100))
        yy += 18
    save(img, "screen_code.png")


def book_spine(size=256):
    """Greyscale spine with bands and a label, tinted per book instance."""
    img = Image.new("RGB", (size, size), (215, 215, 215))
    d = ImageDraw.Draw(img)
    for yy in (24, 40, size - 48, size - 32):
        d.rectangle([0, yy, size, yy + 6], fill=(250, 240, 200))
    d.rectangle([size * 0.2, size * 0.38, size * 0.8, size * 0.6], fill=(245, 240, 225))
    save(img, "book_spine.png")


if __name__ == "__main__":
    floor_wood()
    wallpaper()
    wood()
    fabric_plain()
    blanket()
    rug()
    poster()
    clock_face()
    vinyl()
    screen_code()
    book_spine()
