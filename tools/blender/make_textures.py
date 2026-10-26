"""Generate the station textures (MLI blanket, solar cells, radiator, gold foil) into assets/textures.

Run from the repository root:
    /Applications/Blender.app/Contents/MacOS/Blender --background --python tools/blender/make_textures.py
"""

from pathlib import Path

import bpy
import numpy as np

OUT = Path(__file__).resolve().parents[2] / "assets" / "textures"
SIZE = 1024

def value_noise(size, cells, seed):
    r = np.random.default_rng(seed)
    grid = r.random((cells, cells))
    coords = np.arange(size) * cells / size
    i0 = np.floor(coords).astype(int)
    f = coords - i0
    f = f * f * (3 - 2 * f)
    i1 = (i0 + 1) % cells
    a = grid[np.ix_(i0, i0)]
    b = grid[np.ix_(i0, i1)]
    c = grid[np.ix_(i1, i0)]
    d = grid[np.ix_(i1, i1)]
    fx = f[None, :]
    fy = f[:, None]
    return (a * (1 - fx) + b * fx) * (1 - fy) + (c * (1 - fx) + d * fx) * fy


def fbm(size, base_cells, octaves, seed, gain=0.5):
    total = np.zeros((size, size))
    amplitude = 1.0
    norm = 0.0
    for k in range(octaves):
        total += amplitude * value_noise(size, base_cells * 2 ** k, seed + k)
        norm += amplitude
        amplitude *= gain
    return total / norm


def stretched_noise(size, cells_x, cells_y, seed):
    r = np.random.default_rng(seed)
    grid = r.random((cells_y, cells_x))
    ys = np.arange(size) * cells_y / size
    xs = np.arange(size) * cells_x / size
    y0 = np.floor(ys).astype(int); x0 = np.floor(xs).astype(int)
    fy = ys - y0; fx = xs - x0
    fy = fy * fy * (3 - 2 * fy); fx = fx * fx * (3 - 2 * fx)
    y1 = (y0 + 1) % cells_y; x1 = (x0 + 1) % cells_x
    a = grid[np.ix_(y0, x0)]; b = grid[np.ix_(y0, x1)]
    c = grid[np.ix_(y1, x0)]; d = grid[np.ix_(y1, x1)]
    return (a * (1 - fx[None, :]) + b * fx[None, :]) * (1 - fy[:, None]) + (c * (1 - fx[None, :]) + d * fx[None, :]) * fy[:, None]


def save(name, rgb):
    rgb = np.clip(rgb, 0, 1)
    rgba = np.ones((SIZE, SIZE, 4), dtype=np.float32)
    rgba[..., :3] = rgb[::-1]
    image = bpy.data.images.new(name, SIZE, SIZE, alpha=False)
    image.colorspace_settings.name = "sRGB"
    image.pixels = rgba.ravel()
    image.filepath_raw = f"{OUT}/{name}.png"
    image.file_format = "PNG"
    image.save()
    print("SAVED", name)


def grid_lines(size, spacing, width, offset=0):
    idx = (np.arange(size) + offset) % spacing
    line = (idx < width).astype(float)
    return np.maximum(line[None, :], line[:, None])


def mli_blanket():
    y, x = np.mgrid[0:SIZE, 0:SIZE]
    base = np.array([0.93, 0.92, 0.89])
    panel = 256
    panel_ids = (y // panel) * 4 + (x // panel)
    tint = np.random.default_rng(3).normal(0, 0.025, 16)[panel_ids]
    wrinkles = stretched_noise(SIZE, 6, 48, 11) * 0.6 + stretched_noise(SIZE, 48, 6, 12) * 0.4
    wrinkles = (wrinkles - 0.5) * 0.12
    grime = (fbm(SIZE, 4, 5, 20) - 0.5) * 0.08
    shade = 1.0 + tint + wrinkles + grime
    seam = grid_lines(SIZE, panel, 5, 2)
    dash = ((x // 10 + y // 10) % 2).astype(float)
    stitch_x = ((np.abs((x % panel) - 12) < 2) | (np.abs((x % panel) - (panel - 12)) < 2)) & (dash > 0)
    stitch_y = ((np.abs((y % panel) - 12) < 2) | (np.abs((y % panel) - (panel - 12)) < 2)) & (dash > 0)
    stitches = (stitch_x | stitch_y).astype(float)
    tx = np.abs(((x % panel) - panel / 2 + panel / 4) % (panel / 2) - panel / 4)
    ty = np.abs(((y % panel) - panel / 2 + panel / 4) % (panel / 2) - panel / 4)
    tacks = ((tx ** 2 + ty ** 2) < 16).astype(float)
    edge_dirt = np.exp(-np.minimum(x % panel, panel - x % panel) / 18.0) + np.exp(-np.minimum(y % panel, panel - y % panel) / 18.0)
    shade = shade - edge_dirt * 0.05 - seam * 0.35 - stitches * 0.18 - tacks * 0.25
    save("mli_blanket", base[None, None, :] * shade[..., None])


def solar_array():
    y, x = np.mgrid[0:SIZE, 0:SIZE]
    cell_w, cell_h = 64, 128
    cell_ids = (y // cell_h) * 64 + (x // cell_w)
    variation = np.random.default_rng(5).normal(0, 0.06, 64 * 64)[cell_ids]
    sheen = (fbm(SIZE, 2, 3, 30) - 0.5) * 0.25
    cell = np.array([0.10, 0.11, 0.24])
    rgb = cell[None, None, :] * (1.0 + variation + sheen)[..., None]
    fingers = ((y % 16) == 0).astype(float) * 0.5
    rgb += fingers[..., None] * np.array([0.10, 0.10, 0.12])
    busbar = (np.abs((x % cell_w) - cell_w / 2) < 1.5).astype(float)
    rgb = rgb * (1 - busbar[..., None]) + busbar[..., None] * np.array([0.55, 0.55, 0.58])
    gap = np.maximum(((x % cell_w) < 3).astype(float), ((y % cell_h) < 3).astype(float))
    rgb = rgb * (1 - gap[..., None]) + gap[..., None] * np.array([0.78, 0.72, 0.55])
    string_gap = (((y % 512) < 8)).astype(float)
    rgb = rgb * (1 - string_gap[..., None]) + string_gap[..., None] * np.array([0.85, 0.62, 0.28])
    save("solar_array", rgb)


def radiator():
    y, x = np.mgrid[0:SIZE, 0:SIZE]
    base = np.array([0.90, 0.91, 0.92])
    grime = (fbm(SIZE, 4, 5, 40) - 0.5) * 0.10
    tubes = (np.abs((x % 32) - 16) < 2).astype(float)
    joints = grid_lines(SIZE, 256, 6, 3)
    rivets = (((x % 32) - 16) ** 2 + ((y % 64) - 32) ** 2 < 5).astype(float)
    shade = 1.0 + grime - tubes * 0.10 - joints * 0.30 - rivets * 0.15
    save("radiator", base[None, None, :] * shade[..., None])


def gold_foil():
    a = fbm(SIZE, 8, 6, 50, gain=0.6)
    b = fbm(SIZE, 16, 5, 60, gain=0.55)
    ridges = 1.0 - np.abs(a - 0.5) * 2.0
    facets = np.abs(np.gradient(b, axis=0)) * 40 + np.abs(np.gradient(b, axis=1)) * 40
    bright = 0.70 + ridges * 0.35 + np.clip(facets, 0, 1) * 0.25 - np.clip(1 - ridges, 0, 1) ** 4 * 0.25
    gold = np.array([0.86, 0.62, 0.22])
    save("gold_foil", gold[None, None, :] * bright[..., None])


mli_blanket()
solar_array()
radiator()
gold_foil()
