"""Build the realistic Orbital Dock models into blender/orbital_dock.blend.

Each named model's scene is cleared and rebuilt from code, then the .blend is
saved. Hand edits made in Blender to a rebuilt scene are lost, so only rebuild
the models you mean to regenerate. Export afterwards with export_models.py.
Textures come from assets/textures (make_textures.py generates the new ones).

Run from the repository root, naming one or more models:
    /Applications/Blender.app/Contents/MacOS/Blender blender/orbital_dock.blend --background --python tools/blender/build_models.py -- pod station_hub
"""

import math
import random
import sys
from pathlib import Path

import bmesh
import bpy
from mathutils import Vector

sys.path.insert(0, str(Path(__file__).resolve().parent))
from model_lib import Part, R, T, clear_scene, columns_matrix, frame, material, rock_bmesh  # noqa: E402


def define_materials():
    material("mli", kd=(1.0, 1.0, 1.0), ks=(0.12, 0.12, 0.12), ns=10, texture="mli_blanket")
    material("pod_mli", kd=(0.80, 0.82, 0.76), ks=(0.12, 0.12, 0.12), ns=10, texture="mli_blanket")
    material("pod_dm", kd=(0.55, 0.50, 0.40), ks=(0.15, 0.15, 0.15), ns=12, texture="mli_blanket")
    material("radiator_panel", kd=(1.0, 1.0, 1.0), ks=(0.35, 0.35, 0.35), ns=40, texture="radiator")
    material("gold_foil", kd=(1.0, 1.0, 1.0), ks=(0.9, 0.7, 0.35), ns=90, texture="gold_foil")
    material("solar_cells", kd=(1.0, 1.0, 1.0), ks=(0.7, 0.7, 0.8), ns=160, texture="solar_array")
    material("handrail", kd=(0.86, 0.66, 0.20), ks=(0.5, 0.45, 0.3), ns=48)
    material("cupola_glass", kd=(0.03, 0.04, 0.05), ks=(1.0, 1.0, 1.0), ns=300, ke=(0.22, 0.18, 0.11))
    material("cable", kd=(0.07, 0.07, 0.08), ks=(0.2, 0.2, 0.2), ns=16)
    material("truss", kd=(0.72, 0.73, 0.75), ks=(0.6, 0.6, 0.6), ns=64)
    material("nav_white", kd=(0.9, 0.9, 0.9), ks=(0.5, 0.5, 0.5), ns=32, ke=(0.9, 0.9, 0.95))
    for name in ("metal", "hull_dark", "engine_glow", "pod_glass", "guide_green", "warning_red", "trim_orange", "window_glow"):
        material(name)


def window(part, center, normal, radius, glass="window_glow"):
    m = frame(center, normal)
    part.lathe([(radius, -0.02), (radius, 0.015)], glass, m, 20, cap_start=False, cap_end=True)
    part.torus(radius + 0.03, 0.03, "metal", m @ T(0, 0, 0.01), 20, 6)


def rcs_quad(part, matrix):
    part.box((0.22, 0.22, 0.30), "hull_dark", matrix, bevel=0.02)
    for direction, offset in (((0, 1, 0), 0.11), ((1, 0, 0), 0.11), ((-1, 0, 0), 0.11), ((0, 0, 1), 0.15), ((0, 0, -1), 0.15)):
        d = Vector(direction)
        part.lathe([(0.025, 0.0), (0.035, 0.03), (0.05, 0.07)], "metal", matrix @ frame(d * offset, d), 10)


def build_pod(scene):
    p = Part("pod")
    p.lathe([(0.30, -3.05), (0.28, -3.25), (0.10, -3.45), (0.05, -3.50)], "metal", segments=20, cap_start=True)
    p.tube((0, 0, -3.45), (0, 0, -4.10), 0.045, "metal", 10)
    p.sphere(0.075, "metal", T(0, 0, -4.13), 12, 6)
    p.lathe([(0.36, -2.93), (0.60, -2.96), (0.64, -3.02), (0.64, -3.10), (0.30, -3.12)][::-1], "metal", segments=32)
    p.lathe([(0.55, -2.70), (1.15, -2.78), (1.22, -2.84), (1.22, -2.94), (0.60, -2.98)][::-1], "metal", segments=48)
    for k in range(3):
        a = math.radians(30 + 120 * k)
        d = Vector((math.cos(a), math.sin(a), 0))
        p.ngon_plate([(-0.3, 0.0), (0.3, 0.0), (0.08, 0.38), (-0.08, 0.38)], 0.03, "metal", frame(d * 1.0 + Vector((0, 0, -2.96)), -d, (0, 0, -1)) @ R("X", 20))
    for k in range(3):
        a = math.radians(90 + 120 * k)
        d = Vector((math.cos(a), math.sin(a), 0))
        p.box((0.12, 0.05, 0.20), "trim_orange", frame(d * 0.66 + Vector((0, 0, -3.05)), (0, 0, 1), d), bevel=0.01)

    center = -2.0
    radius = 1.05
    om = []
    for i in range(15):
        t = math.radians(150 - 120 * i / 14)
        om.append((radius * math.sin(t), center + radius * math.cos(t) * -1.0))
    om.sort(key=lambda rz: rz[1])
    om = [(0.36, -2.93)] + om + [(0.48, -1.0)]
    p.lathe(om, "pod_mli", segments=36, tile=1.2)
    window(p, Vector((0, 0, center)) + Vector((0, 0.9945, -0.1047)) * (radius + 0.005), (0, 0.9945, -0.1047), 0.15, "pod_glass")
    window(p, Vector((0, 0, center)) + Vector((0.866, 0.30, -0.40)).normalized() * (radius + 0.005), (0.866, 0.30, -0.40), 0.11, "pod_glass")
    boom_base = Vector((0, 0, center)) + Vector((0, 0.55, -0.83)).normalized() * radius
    p.tube(boom_base, boom_base + Vector((0, 0.55, -0.35)), 0.025, "metal", 6)
    tip = boom_base + Vector((0, 0.55, -0.35))
    for k in range(3):
        a = math.radians(120 * k)
        p.tube(tip, tip + Vector((math.cos(a) * 0.32, 0.05, math.sin(a) * 0.32)), 0.012, "metal", 5)
    for a in (40, 140, 220, 320):
        d = Vector((math.cos(math.radians(a)), math.sin(math.radians(a)), 0))
        p0 = Vector((0, 0, center - 0.25)) + d * (radius * 0.99)
        p1 = Vector((0, 0, center + 0.35)) + d * (radius * 0.98)
        p.handrail(p0, p1, d, height=0.08, radius=0.018)

    p.lathe([(0.44, -1.12), (0.44, -0.86)], "metal", segments=28)
    dm = [(0.44, -0.90), (0.62, -0.84), (0.84, -0.62), (0.99, -0.30), (1.07, 0.05), (1.10, 0.35), (1.10, 0.55)]
    p.lathe(dm, "pod_dm", segments=40, tile=1.2)
    window(p, (0, 0.98, -0.22), (0, 1, -0.32), 0.09, "pod_glass")
    p.box((0.16, 0.12, 0.28), "metal", frame((0.32, 0.95, -0.35), (0, 0.35, 1)), bevel=0.02)
    p.lathe([(1.10, 0.55), (1.16, 0.60), (1.16, 0.70), (1.12, 0.73)], "metal", segments=40)

    sm = [(1.12, 0.73), (1.12, 1.25), (1.18, 1.35), (1.30, 2.22), (1.30, 2.36)]
    p.lathe(sm, "radiator_panel", segments=40, tile=1.0)
    p.lathe([(1.305, 1.98), (1.305, 2.28)], "gold_foil", segments=40, tile=0.8)
    p.lathe([(1.30, 2.36), (0.95, 2.40), (0.40, 2.42)], "hull_dark", segments=40, cap_end=True)
    for k in range(8):
        a = math.radians(22.5 + 45 * k)
        d = Vector((math.cos(a), math.sin(a), 0))
        p.box((0.05, 0.04, 1.02), "metal", frame(d * 1.255 + Vector((0, 0, 1.80)), (0, 0, 1), d))
    for k in range(4):
        a = math.radians(45 + 90 * k)
        d = Vector((math.cos(a), math.sin(a), 0))
        rcs_quad(p, frame(d * 1.36 + Vector((0, 0, 1.75)), (0, 0, 1), d))
    p.sphere(0.045, "guide_green", T(1.33, 0.0, 1.50), 10, 5)
    p.sphere(0.045, "warning_red", T(-1.33, 0.0, 1.50), 10, 5)
    p.sphere(0.035, "nav_white", T(0.0, 1.32, 2.30), 10, 5)
    p.tube((0.0, -1.24, 1.0), (0.0, -1.85, 1.15), 0.015, "metal", 5)
    p.tube((0.9, -0.9, 2.2), (1.25, -1.35, 2.45), 0.012, "metal", 5)

    p.lathe([(0.15, 2.40), (0.20, 2.48), (0.31, 2.66), (0.35, 2.73)], "metal", segments=28)
    p.lathe([(0.33, 2.72), (0.29, 2.66), (0.18, 2.49), (0.12, 2.43)], "engine_glow", segments=28, cap_end=True)
    for k in range(4):
        a = math.radians(45 + 90 * k)
        c = Vector((math.cos(a) * 0.62, math.sin(a) * 0.62, 0))
        p.lathe([(0.05, 2.40), (0.07, 2.46), (0.10, 2.55)], "metal", T(*c), 14)
        p.lathe([(0.09, 2.55), (0.06, 2.47), (0.04, 2.42)], "hull_dark", T(*c), 14, cap_end=True)
    p.finish(scene)


def build_station_hub(scene):
    h = Part("station_hub")
    body_tiles = round(2 * math.pi * 5.6 / 4.0)
    h.lathe([(5.6, -12.3), (5.6, 12.0)], "mli", segments=64, tile=4.0, u_tiles=body_tiles)
    h.lathe([(3.2, -13.4), (4.6, -13.05), (5.6, -12.3)], "mli", segments=64, tile=4.0, u_tiles=body_tiles)
    h.lathe([(5.6, 12.0), (5.0, 12.6), (3.4, 13.0)], "mli", segments=64, tile=4.0, u_tiles=body_tiles, cap_end=False)
    h.lathe([(3.4, 13.0), (3.0, 13.02)], "metal", segments=64, cap_end=True)
    for z in (-9.0, -3.0, 3.0, 9.0):
        h.lathe([(5.6, z - 0.14), (5.68, z - 0.10), (5.68, z + 0.10), (5.6, z + 0.14)], "metal", segments=64)
    for z in (-12.3, 12.0):
        h.lathe([(5.6, z - 0.08), (5.66, z - 0.05), (5.66, z + 0.05), (5.6, z + 0.08)], "metal", segments=64)

    rng = random.Random(4)
    for angle in (20, 160, 200, 250, 290, 340):
        d = Vector((math.cos(math.radians(angle)), math.sin(math.radians(angle)), 0))
        for z0 in (-11.0, -7.4, -1.6, 4.4, 10.0):
            if rng.random() < 0.2:
                continue
            h.handrail(d * 5.6 + Vector((0, 0, z0)), d * 5.6 + Vector((0, 0, z0 + 1.2)), d)
    for angle in (90 - 25, 90 + 25):
        d = Vector((math.cos(math.radians(angle)), math.sin(math.radians(angle)), 0))
        for z0 in (-11.0, -5.0, 1.0, 7.0):
            h.handrail(d * 5.6 + Vector((0, 0, z0)), d * 5.6 + Vector((0, 0, z0 + 1.0)), d)

    for angle, z0, z1 in ((0, -11.5, 11.5), (180, -11.5, 11.5)):
        d = Vector((math.cos(math.radians(angle)), math.sin(math.radians(angle)), 0))
        h.box_span((-0.2, -0.12, z0), (0.2, 0.12, z1), "hull_dark", frame(d * 5.72, (0, 0, 1), d), bevel=0.03)
        for z in range(int(z0) + 1, int(z1), 2):
            h.box((0.5, 0.06, 0.05), "metal", frame(d * 5.72 + Vector((0, 0, z)), (0, 0, 1), d))

    orus = [(25, -6.0, (1.4, 0.7, 1.0)), (335, 2.5, (1.1, 0.6, 1.6)), (205, 6.0, (1.2, 0.8, 1.2)), (155, -3.5, (0.9, 0.5, 1.3))]
    for angle, z, size in orus:
        d = Vector((math.cos(math.radians(angle)), math.sin(math.radians(angle)), 0))
        m = frame(d * (5.6 + size[1] / 2) + Vector((0, 0, z)), (0, 0, 1), d)
        h.box((size[0], size[1], size[2]), "mli", m, bevel=0.05, tile=1.5)
        h.handrail(d * (5.6 + size[1]) + Vector((0, 0, z - size[2] / 2 + 0.15)), d * (5.6 + size[1]) + Vector((0, 0, z + size[2] / 2 - 0.15)), d, height=0.08)

    for angle, z in ((300, -4.0), (60, 7.0)):
        d = Vector((math.cos(math.radians(angle)), math.sin(math.radians(angle)), 0))
        m = frame(d * 5.62 + Vector((0, 0, z)), d)
        h.box((0.9, 0.9, 0.12), "metal", m @ T(0, 0, 0.06), bevel=0.02)
        h.cylinder(0.12, 0.45, "metal", m @ T(0, 0, 0.32), 12)
        for k in range(3):
            a = math.radians(120 * k)
            h.box((0.12, 0.12, 0.25), "hull_dark", m @ T(math.cos(a) * 0.32, math.sin(a) * 0.32, 0.25), bevel=0.01)

    for z in (-8.0, -5.0, 6.0, 9.0):
        d = Vector((math.cos(math.radians(330)), math.sin(math.radians(330)), 0))
        window(h, d * 5.6 + Vector((0, 0, z)), d, 0.22)

    cupola = frame((0, -5.45, 4.0), (0, -1, 0))
    h.lathe([(1.55, 0.0), (1.55, 0.45), (1.45, 0.55)], "metal", cupola, 6, tile=1.0)
    h.lathe([(1.45, 0.55), (0.95, 1.05)], "cupola_glass", cupola, 6)
    h.lathe([(0.95, 1.05), (0.75, 1.10)], "metal", cupola, 6)
    h.lathe([(0.62, 1.10), (0.62, 1.12)], "cupola_glass", cupola, 24, cap_end=True)
    h.torus(0.68, 0.06, "metal", cupola @ T(0, 0, 1.12), 24, 6)
    for k in range(6):
        a = math.radians(60 * k)
        h.tube(cupola @ Vector((math.cos(a) * 1.46, math.sin(a) * 1.46, 0.55)), cupola @ Vector((math.cos(a) * 0.96, math.sin(a) * 0.96, 1.06)), 0.04, "metal", 6)
        h.box((0.18, 0.18, 0.35), "hull_dark", cupola @ frame((math.cos(a + 0.52) * 1.6, math.sin(a + 0.52) * 1.6, 0.25), (0, 0, 1)), bevel=0.02)

    for k in range(4):
        a = math.radians(45 + 90 * k)
        d = Vector((math.cos(a), math.sin(a), 0))
        m = frame(d * 5.2, d)
        h.lathe([(1.3, 0.0), (1.3, 0.9), (1.15, 1.0)], "metal", m, 32)
        h.lathe([(1.15, 1.0), (1.0, 1.05)], "hull_dark", m, 32, cap_end=True)
        for j in range(8):
            b = math.radians(45 * j)
            h.box((0.12, 0.2, 0.25), "hull_dark", m @ T(math.cos(b) * 1.32, math.sin(b) * 1.32, 0.6) @ R("Z", 45 * j), bevel=0.015)

    for x in (-1.7, 1.7):
        h.box_span((x - 0.18, -0.8, 12.6), (x + 0.18, 0.8, 13.0), "hull_dark", bevel=0.03)
        h.box_span((x - 0.2, -0.55, 13.0), (x + 0.2, 0.55, 13.35), "metal", bevel=0.04)
        h.cylinder(0.32, 0.42, "metal", T(x, 0, 13.0) @ R("Y", 90), 20)
    h.box_span((-2.2, -1.1, 12.95), (2.2, 1.1, 13.05), "metal", bevel=0.02)

    h.lathe([(3.0, -21.0), (3.0, -13.3)], "mli", segments=48, tile=3.0, u_tiles=round(2 * math.pi * 3.0 / 3.0))
    h.lathe([(3.0, -21.0), (2.6, -21.3), (1.6, -21.5)][::-1], "metal", segments=48)
    h.lathe([(1.6, -21.5), (0.95, -21.5)][::-1], "hull_dark", segments=32)
    h.lathe([(0.95, -21.8), (0.95, -21.5)], "metal", segments=32, cap_start=True)
    for z in (-19.5, -16.5, -14.5):
        h.lathe([(3.0, z - 0.1), (3.07, z - 0.06), (3.07, z + 0.06), (3.0, z + 0.1)], "metal", segments=48)
    for y_sign in (1, -1):
        h.box_span((-0.3, y_sign * 2.9, -19.6), (0.3, y_sign * 3.4, -15.0), "hull_dark", bevel=0.04)
        lo = (-0.05, 3.4, -20.0) if y_sign > 0 else (-0.05, -8.6, -20.0)
        hi = (0.05, 8.6, -14.6) if y_sign > 0 else (0.05, -3.4, -14.6)
        h.box_span(lo, hi, "radiator_panel", tile=2.6, bevel=0.01)
        for z in (-19.4, -15.2):
            h.tube((0, y_sign * 3.3, z), (0, y_sign * 8.5, z), 0.06, "truss", 8)
    for angle in (25, 155, 210, 330):
        d = Vector((math.cos(math.radians(angle)), math.sin(math.radians(angle)), 0))
        h.handrail(d * 3.0 + Vector((0, 0, -20.2)), d * 3.0 + Vector((0, 0, -19.0)), d)
        h.handrail(d * 3.0 + Vector((0, 0, -15.8)), d * 3.0 + Vector((0, 0, -14.6)), d)
    for x_sign in (1, -1):
        m = frame((x_sign * 3.0, 0, -17.0), (x_sign, 0, 0))
        h.lathe([(0.9, 0.0), (0.9, 0.25), (0.7, 0.35)], "metal", m, 24, cap_end=True)

    dish_base = Vector((-2.1, 2.1, -20.5))
    h.tube(dish_base, dish_base + Vector((-0.9, 0.9, -0.6)), 0.06, "truss", 8)
    dish = frame(dish_base + Vector((-0.9, 0.9, -0.6)), (-0.5, 0.4, -1.0))
    h.lathe([(0.05 + 0.75 * (i / 6), 0.25 * (i / 6) ** 2) for i in range(7)], "metal", dish, 24)
    h.lathe([(0.05 + 0.75 * (i / 6), 0.25 * (i / 6) ** 2 - 0.02) for i in range(7)][::-1], "metal", dish, 24)
    h.tube(dish @ Vector((0, 0, 0)), dish @ Vector((0, 0, 0.45)), 0.02, "metal", 6)
    h.finish(scene)


RING_RADIUS = 34.0
RING_MODULE_RADIUS = 2.2


def ring_point(degrees, radius=RING_RADIUS):
    a = math.radians(degrees)
    return Vector((math.cos(a) * radius, math.sin(a) * radius, 0.0))


def build_station_ring(scene):
    r = Part("station_ring")
    spoke_nodes = {3, 9, 15, 21}
    for i in range(24):
        a0, a1 = 15.0 * i, 15.0 * (i + 1)
        p0, p1 = ring_point(a0), ring_point(a1)
        chord = p1 - p0
        center = (p0 + p1) / 2
        outward = center.normalized()
        half = chord.length / 2 - 0.55
        m = frame(center, chord, (0, 0, 1))
        tiles = round(2 * math.pi * RING_MODULE_RADIUS / 4.0)
        r.lathe([(RING_MODULE_RADIUS, -half), (RING_MODULE_RADIUS, half)], "mli", m, 32, tile=4.0, u_tiles=tiles)
        for z in (-half + 0.4, half - 0.4):
            r.lathe([(2.2, z - 0.08), (2.26, z - 0.05), (2.26, z + 0.05), (2.2, z + 0.08)], "metal", m, 24)
        for side in (1, -1):
            for k in range(4):
                offset = -half + 1.0 + k * (2 * half - 2.0) / 3
                pos = center + chord.normalized() * offset + Vector((0, 0, side * RING_MODULE_RADIUS * 0.97)) + outward * 0.5
                normal = (outward * 0.25 + Vector((0, 0, side))).normalized()
                r.box((0.34, 0.24, 0.05), "window_glow", frame(pos, normal, chord))
                r.box((0.42, 0.32, 0.03), "metal", frame(pos - normal * 0.02, normal, chord))
        r.handrail(center - chord.normalized() * 1.5 + outward * 2.2, center + chord.normalized() * 1.5 + outward * 2.2, outward)
        r.box((1.0, 0.5, 0.8), "hull_dark", frame(center - outward * 2.35 + chord.normalized() * 1.2, chord, -outward), bevel=0.04)
        r.box((0.7, 0.3, 1.6), "radiator_panel", frame(center + outward * 2.3 - chord.normalized() * 1.6, outward, (0, 0, 1)) @ T(0, 0, 0.1), bevel=0.02, tile=1.5)

    for i in range(24):
        a = 15.0 * i
        tangent = Vector((-math.sin(math.radians(a)), math.cos(math.radians(a)), 0.0))
        m = frame(ring_point(a), tangent, (0, 0, 1))
        if i in spoke_nodes:
            r.sphere(2.55, "mli", m, 28, 12, tile=4.0)
            r.lathe([(2.6, -0.5), (2.62, -0.45), (2.62, 0.45), (2.6, 0.5)], "metal", m, 32)
        else:
            r.lathe([(2.2, -0.62), (2.45, -0.5), (2.45, 0.5), (2.2, 0.62)], "metal", m, 32)
            for k in range(8):
                b = math.radians(45 * k + 22.5)
                r.box((0.16, 0.12, 0.7), "hull_dark", m @ T(math.cos(b) * 2.48, math.sin(b) * 2.48, 0) @ R("Z", 45 * k + 22.5))

    for k in range(4):
        a = 45.0 + 90.0 * k
        d = ring_point(a, 1.0)
        start, end = d * 6.3, d * 31.6
        r.tube(start, end, 0.75, "mli", 24, caps=False, tile=3.0)
        for t in range(1, 6):
            pos = start + (end - start) * (t / 6)
            r.lathe([(0.75, -0.1), (0.82, -0.06), (0.82, 0.06), (0.75, 0.1)], "metal", frame(pos, d), 20)
        side = Vector((0, 0, 1))
        for z in (1.0, -1.0):
            r.tube(start + side * z + d * 0.4, end + side * z - d * 0.6, 0.07, "truss", 8)
        steps = 12
        for t in range(steps):
            p0 = start + (end - start) * (t / steps) + d * 0.4
            p1 = start + (end - start) * ((t + 1) / steps) + d * 0.4
            r.tube(p0 + side, p1 - side, 0.035, "truss", 6, caps=False)
            r.tube(p0 + side, p0 - side, 0.035, "truss", 6, caps=False)
        r.lathe([(1.15, 0.0), (1.15, 0.5), (0.8, 0.7)], "metal", frame(d * 31.0, d * -1), 24)
        r.handrail(d * 12 + Vector((0.0, 0.0, 0.0)) + d.cross(side) * 0.75, d * 14 + d.cross(side) * 0.75, d.cross(side))
        r.handrail(d * 22 + d.cross(side) * 0.75, d * 24 + d.cross(side) * 0.75, d.cross(side))
    r.finish(scene)


def build_station_solar(scene):
    s = Part("station_solar")
    for sign in (1, -1):
        x_dir = Vector((sign, 0, 0))
        s.lathe([(0.75, 0.0), (0.75, 0.45)], "metal", frame((sign * 3.3, 0, -17.0), x_dir), 24, cap_end=True)
        for y in (-0.4, 0.4):
            for z in (-17.4, -16.6):
                s.tube((sign * 3.7, y, z), (sign * 7.2, y, z), 0.05, "truss", 6)
        for t in range(5):
            x = sign * (3.7 + 0.875 * t)
            for (y0, z0), (y1, z1) in (((-0.4, -17.4), (0.4, -17.4)), ((0.4, -17.4), (0.4, -16.6)), ((0.4, -16.6), (-0.4, -16.6)), ((-0.4, -16.6), (-0.4, -17.4)), ((-0.4, -17.4), (0.4, -16.6))):
                s.tube((x, y0, z0), (x, y1, z1), 0.03, "truss", 5, caps=False)
        s.box_span((min(sign * 7.2, sign * 8.1), -0.28, -17.6), (max(sign * 7.2, sign * 8.1), 0.28, -16.4), "hull_dark", bevel=0.04)
        s.box_span((min(sign * 39.6, sign * 40.0), -0.2, -24.4), (max(sign * 39.6, sign * 40.0), 0.2, -9.6), "metal", bevel=0.03)
        for blanket_z0, blanket_z1 in ((-24.3, -17.65), (-16.35, -9.7)):
            s.box_span((min(sign * 8.1, sign * 8.4), -0.18, blanket_z0), (max(sign * 8.1, sign * 8.4), 0.18, blanket_z1), "hull_dark", bevel=0.02)
            panels = 16
            length = 39.5 - 8.5
            for k in range(panels):
                x0 = 8.5 + length * k / panels + 0.03
                x1 = 8.5 + length * (k + 1) / panels - 0.03
                lo, hi = sorted((sign * x0, sign * x1))
                s.box_span((lo, 0.0, blanket_z0), (hi, 0.012, blanket_z1), "solar_cells", tile=2.0)
                s.box_span((lo, -0.012, blanket_z0), (hi, 0.0, blanket_z1), "gold_foil", tile=2.0)
        for k in range(32):
            x = sign * (8.1 + (39.6 - 8.1) * k / 32)
            x1 = sign * (8.1 + (39.6 - 8.1) * (k + 1) / 32)
            for (ya, za), (yb, zb) in (((0.22, -17.0), (-0.11, -16.81)), ((-0.11, -16.81), (-0.11, -17.19)), ((-0.11, -17.19), (0.22, -17.0))):
                s.tube((x, ya, za), (x, yb, zb), 0.018, "truss", 4, caps=False)
            for ya, za in ((0.22, -17.0), (-0.11, -16.81), (-0.11, -17.19)):
                s.tube((x, ya, za), (x1, ya, za), 0.022, "truss", 4, caps=False)
            s.tube((x, 0.22, -17.0), (x1, -0.11, -16.81), 0.012, "truss", 4, caps=False)
    s.finish(scene)


def build_docking_arm(scene):
    a = Part("docking_arm")
    a.cylinder(0.42, 2.9, "metal", R("Y", 90), 28)
    for x in (-1.45, 1.45):
        a.cylinder(0.3, 0.06, "hull_dark", T(x, 0, 0) @ R("Y", 90), 20)
    a.box_span((-1.2, -0.8, 0.0), (1.2, 0.8, 1.05), "hull_dark", bevel=0.06)
    a.lathe([(1.05, 1.05), (0.85, 1.3)], "metal", segments=32)
    a.lathe([(0.85, 1.3), (0.85, 9.3)], "mli", segments=32, tile=2.0, u_tiles=3)
    for z in [1.3 + 1.0 * k for k in range(1, 8)]:
        a.lathe([(0.85, z - 0.06), (0.9, z - 0.04), (0.9, z + 0.04), (0.85, z + 0.06)], "metal", segments=32)
    a.lathe([(0.85, 9.3), (1.2, 9.75), (1.3, 9.8), (1.3, 10.0)], "metal", segments=32, cap_end=True)
    for k in range(12):
        b = math.radians(30 * k)
        a.cylinder(0.04, 0.06, "hull_dark", T(math.cos(b) * 1.18, math.sin(b) * 1.18, 10.02), 8)
    for angle in (30, 150, 270):
        d = Vector((math.cos(math.radians(angle)), math.sin(math.radians(angle)), 0))
        for z0 in (2.0, 4.6, 7.2):
            a.handrail(d * 0.85 + Vector((0, 0, z0)), d * 0.85 + Vector((0, 0, z0 + 1.6)), d, height=0.1)
    d = Vector((math.cos(math.radians(90)), math.sin(math.radians(90)), 0))
    a.tube(d * 0.98 + Vector((0, 0, 1.4)), d * 0.98 + Vector((0, 0, 9.2)), 0.07, "cable", 8)
    for z in (2.5, 4.5, 6.5, 8.5):
        a.box((0.22, 0.1, 0.08), "metal", T(0, 0.95, z))
    a.box((0.5, 0.3, 0.7), "mli", frame(Vector((0.0, -0.98, 5.0)), (0, 0, 1), (0, -1, 0)), bevel=0.04, tile=1.0)
    a.finish(scene)


def build_docking_port(scene):
    p = Part("docking_port")
    p.lathe([(1.35, -0.8), (1.35, -0.45), (2.3, -0.05)], "mli", segments=48, cap_start=True, tile=2.0)
    p.lathe([(2.3, -0.05), (2.83, 0.05), (2.83, 0.85), (2.6, 1.0)], "metal", segments=48)
    p.lathe([(2.84, 0.3), (2.84, 0.55)], "port_yellow", segments=48)
    p.lathe([(2.6, 1.0), (1.95, 1.0)], "metal", segments=48)
    p.lathe([(1.95, 1.0), (1.95, 1.12), (1.80, 1.25), (1.62, 1.25), (1.62, 1.0)], "port_yellow", segments=48)
    p.lathe([(1.62, 1.0), (1.2, 0.75), (0.55, 0.38), (0.14, 0.15)], "hull_dark", segments=40)
    p.lathe([(0.14, 0.15), (0.14, 0.05)][::-1], "metal", segments=16, cap_end=False)
    for k in range(3):
        b = math.radians(90 + 120 * k)
        d = Vector((math.cos(b), math.sin(b), 0))
        p.ngon_plate([(-0.45, 0.0), (0.45, 0.0), (0.12, 0.55), (-0.12, 0.55)], 0.04, "metal", frame(d * 1.82 + Vector((0, 0, 1.18)), d, (0, 0, 1)) @ R("X", -25))
    for k in range(12):
        b = math.radians(15 + 30 * k)
        d = Vector((math.cos(b), math.sin(b), 0))
        p.box((0.16, 0.22, 0.1), "hull_dark", frame(d * 2.25 + Vector((0, 0, 1.03)), (0, 0, 1), d), bevel=0.02)
    for k in range(4):
        b = math.radians(90 * k + 90)
        d = Vector((math.cos(b), math.sin(b), 0))
        p.box((0.6, 0.32, 0.06), "cable", frame(d * 2.6 + Vector((0, 0, 1.0)), (0, 0, 1), d))
    for k in range(4):
        b = math.radians(90 * k + 90)
        d = Vector((math.cos(b), math.sin(b), 0))
        lamp = d * 2.75 + Vector((0, 0, 1.12)) + d.cross(Vector((0, 0, 1))) * 0.55
        p.box((0.16, 0.12, 0.16), "hull_dark", frame(lamp, (0, 0, 1), d), bevel=0.02)
        p.sphere(0.06, "guide_green", T(*(lamp + Vector((0, 0, 0.1)))), 10, 5)
    target = frame(Vector((math.cos(math.radians(135)), math.sin(math.radians(135)), 0)) * 3.15 + Vector((0, 0, 0.9)), (0, 0, 1))
    p.tube(Vector((math.cos(math.radians(135)), math.sin(math.radians(135)), 0)) * 2.83 + Vector((0, 0, 0.6)), target @ Vector((0, 0, 0)), 0.04, "metal", 6)
    p.cylinder(0.32, 0.03, "cable", target, 24)
    p.box((0.5, 0.06, 0.02), "nav_white", target @ T(0, 0, 0.02))
    p.box((0.06, 0.5, 0.02), "nav_white", target @ T(0, 0, 0.02))
    p.tube(target @ Vector((0, 0, 0.0)), target @ Vector((0, 0, 0.3)), 0.025, "nav_white", 8)
    p.box((0.16, 0.03, 0.02), "nav_white", target @ T(0, 0, 0.31))
    p.finish(scene)


def build_docking_clamp(scene):
    c = Part("docking_clamp")
    c.cylinder(0.15, 0.46, "metal", R("Y", 90), 16)
    profile = [(-0.12, -0.15), (0.15, -0.15), (0.32, 0.6), (0.30, 1.42), (0.12, 1.65), (-0.05, 1.65), (-0.05, 1.50), (0.12, 1.40), (0.13, 0.62), (-0.12, 0.12)]
    swap = columns_matrix((0, 1, 0), (0, 0, 1), (1, 0, 0))
    c.ngon_plate(profile, 0.3, "hull_dark", T(-0.15, 0, 0) @ swap)
    c.box_span((-0.16, -0.06, 1.48), (0.16, 0.05, 1.66), "trim_orange", bevel=0.01)
    c.tube((0, 0.28, 0.3), (0, 0.24, 1.1), 0.035, "metal", 8)
    c.finish(scene)


def build_eva_deck(scene):
    d = Part("eva_deck")
    deck_top = 6.6
    for x in (-1.6, 1.6):
        d.box_span((x - 0.12, 5.7, -11.7), (x + 0.12, 5.86, 11.0), "truss")
        d.box_span((x - 0.12, 6.34, -11.7), (x + 0.12, 6.5, 11.0), "truss")
        z = -11.4
        while z < 10.8:
            d.tube((x, 5.86, z), (x, 6.34, z), 0.04, "truss", 6, caps=False)
            if z + 1.0 <= 11.0:
                d.tube((x, 5.86, z), (x, 6.34, z + 1.0), 0.03, "truss", 6, caps=False)
            z += 1.0
    z = -11.0
    while z < 11.0:
        d.box_span((-1.6, 6.36, z - 0.06), (1.6, 6.48, z + 0.06), "truss")
        d.tube((-1.6, 5.75, z), (-0.9, 5.62, z), 0.06, "metal", 8)
        d.tube((1.6, 5.75, z), (0.9, 5.62, z), 0.06, "metal", 8)
        z += 2.0
    d.box_span((-2.2, 6.5, -10.55), (2.2, deck_top, 11.0), "deck", tile=1.0)
    for x in (-2.2, 2.2):
        d.box_span((x - 0.03, 6.45, -10.55), (x + 0.03, 6.72, 11.0), "trim_orange")
    for x in (-2.12, 2.12):
        z = -10.3
        while z <= 10.8:
            d.tube((x, deck_top, z), (x, 7.75, z), 0.03, "metal", 8)
            z += 1.85
        d.tube((x, 7.2, -10.4), (x, 7.2, 10.9), 0.028, "handrail", 8)
        d.tube((x, 7.75, -10.4), (x, 7.75, 10.9), 0.032, "handrail", 8)
    d.tube((-2.12, 7.75, 10.9), (2.12, 7.75, 10.9), 0.032, "handrail", 8)
    d.tube((-2.12, 7.2, 10.9), (2.12, 7.2, 10.9), 0.028, "handrail", 8)
    d.box_span((-2.2, deck_top, -11.7), (2.2, 9.7, -10.55), "mli", bevel=0.05, tile=2.0)
    d.box_span((-2.25, 9.7, -11.75), (2.25, 9.78, -10.5), "metal", bevel=0.02)
    d.box_span((-0.2, 9.78, -11.3), (0.2, 9.85, -10.9), "hull_dark", bevel=0.01)
    for (x0, x1, y0, y1) in ((-0.8, -0.68, 7.2, 8.8), (0.68, 0.8, 7.2, 8.8), (-0.8, 0.8, 7.2, 7.32), (-0.8, 0.8, 8.68, 8.8)):
        d.box_span((x0, y0, -10.55), (x1, y1, -10.45), "metal", bevel=0.01)
    for x in (-1.6, -1.25, 1.25, 1.6):
        d.cylinder(0.08, 0.1, "hull_dark", T(x, 7.6, -10.5), 16)
        d.cylinder(0.05, 0.12, "metal", T(x, 7.6, -10.48), 12)
    for x, mat in ((-1.5, "status_green"), (-1.3, "status_green"), (1.3, "status_green"), (1.5, "trim_orange")):
        d.box((0.08, 0.05, 0.03), mat, T(x, 9.2, -10.54))
    d.box_span((1.1, 8.3, -10.56), (1.9, 8.9, -10.53), "nav_white")
    d.box_span((-1.9, 8.3, -10.56), (-1.1, 8.9, -10.53), "nav_white")
    d.handrail((-1.9, 9.4, -10.55), (-1.0, 9.4, -10.55), (0, 0, 1), height=0.1)
    d.handrail((1.0, 9.4, -10.55), (1.9, 9.4, -10.55), (0, 0, 1), height=0.1)
    d.box_span((-0.35, deck_top, -9.9), (0.35, deck_top + 0.05, -9.3), "metal", bevel=0.01)
    for x in (-0.18, 0.18):
        d.polyline_tube([(x - 0.08, deck_top + 0.05, -9.55), (x - 0.08, deck_top + 0.14, -9.5), (x + 0.08, deck_top + 0.14, -9.5), (x + 0.08, deck_top + 0.05, -9.55)], 0.015, "handrail", 6)
    d.box_span((1.5, deck_top, 4.0), (2.0, deck_top + 0.45, 4.8), "hull_dark", bevel=0.03)
    d.handrail((1.55, deck_top + 0.45, 4.2), (1.55, deck_top + 0.45, 4.6), (0, 1, 0), height=0.08)
    d.box_span((-2.05, deck_top, -3.0), (-1.55, deck_top + 0.6, -2.2), "mli", bevel=0.03, tile=1.0)
    d.tube((-2.15, 6.72, -10.3), (-2.15, 6.72, 10.8), 0.035, "cable", 8)
    d.finish(scene)


def build_hull_panel(scene):
    h = Part("hull_panel")
    h.box_span((-0.68, 0.0, -0.68), (0.68, 0.055, 0.68), "panel_ok", bevel=0.02, tile=1.4)
    h.box_span((-0.6, 0.055, -0.6), (0.6, 0.06, 0.6), "panel_ok", tile=1.4)
    for x in (-0.6, -0.2, 0.2, 0.6):
        for z in (-0.6, 0.6):
            h.cylinder(0.022, 0.02, "metal", T(x, 0.065, z) @ R("X", 90), 8)
            h.cylinder(0.022, 0.02, "metal", T(z, 0.065, x) @ R("X", 90), 8)
    h.handrail((-0.2, 0.06, 0.3), (0.2, 0.06, 0.3), (0, 1, 0), height=0.03, radius=0.012)
    h.box_span((-0.35, 0.06, -0.38), (0.35, 0.064, -0.30), "trim_orange")
    h.box_span((-0.5, 0.06, -0.12), (-0.1, 0.063, -0.05), "nav_white")
    h.finish(scene)


def panel_base(p, with_hole):
    if with_hole:
        hole = (0.10, -0.05, 0.26)
        x0, y0, half = hole
        for lo, hi in (((-0.62, -0.62), (0.62, y0 - half)), ((-0.62, y0 + half), (0.62, 0.62)), ((-0.62, y0 - half), (x0 - half, y0 + half)), ((x0 + half, y0 - half), (0.62, y0 + half))):
            p.box_span((lo[0], lo[1], -0.06), (hi[0], hi[1], 0.02), "panel_ok", tile=1.3)
        p.box_span((x0 - half, y0 - half, -0.06), (x0 + half, y0 + half, -0.05), "cable")
    else:
        p.box_span((-0.62, -0.62, -0.06), (0.62, 0.62, 0.02), "panel_ok", bevel=0.015, tile=1.3)
    p.box_span((-0.65, -0.65, -0.06), (-0.6, 0.65, 0.035), "metal")
    p.box_span((0.6, -0.65, -0.06), (0.65, 0.65, 0.035), "metal")
    p.box_span((-0.6, -0.65, -0.06), (0.6, -0.6, 0.035), "metal")
    p.box_span((-0.6, 0.6, -0.06), (0.6, 0.65, 0.035), "metal")


def build_repair_panel_fixed(scene):
    p = Part("repair_panel_fixed")
    panel_base(p, False)
    for x in (-0.52, 0.52):
        for y in (-0.52, 0.52):
            p.cylinder(0.035, 0.03, "metal", T(x, y, 0.035), 12)
            p.box((0.04, 0.008, 0.01), "hull_dark", T(x, y, 0.052))
    for k in range(4):
        x = -0.3 + 0.2 * k
        p.cylinder(0.055, 0.04, "hull_dark", T(x, -0.38, 0.04), 16)
        p.cylinder(0.035, 0.045, "metal", T(x, -0.38, 0.045), 12)
    p.box_span((-0.25, 0.42, 0.02), (0.25, 0.47, 0.035), "status_green")
    p.box_span((-0.45, 0.1, 0.02), (0.45, 0.25, 0.024), "nav_white")
    p.box_span((-0.45, -0.05, 0.02), (0.0, 0.0, 0.024), "trim_orange")
    p.finish(scene)


def build_repair_panel_damaged(scene):
    p = Part("repair_panel_damaged")
    panel_base(p, True)
    x0, y0, half = 0.10, -0.05, 0.26
    rnd = random.Random(9)
    for k in range(7):
        b = math.radians(360 / 7 * k + rnd.uniform(-10, 10))
        d = Vector((math.cos(b), math.sin(b), 0))
        base = Vector((x0, y0, 0.02)) + d * 0.26
        tangent = Vector((-d.y, d.x, 0))
        tip = Vector((x0, y0, 0.02)) + d * rnd.uniform(0.33, 0.42) + Vector((0, 0, rnd.uniform(0.06, 0.16)))
        width = rnd.uniform(0.07, 0.11)
        mat = "scorched" if k % 2 else "panel_ok"
        verts = [base - tangent * width, base + tangent * width, tip]
        temp = bmesh.new()
        temp.faces.new([temp.verts.new(v) for v in verts])
        temp.faces.new([temp.verts.new(v - Vector((0, 0, 0.004))) for v in verts[::-1]])
        p.add_bmesh(temp, mat, tile=1.0)
    p.lathe([(0.48, 0.021), (0.27, 0.024)], "scorched", T(x0, y0, 0), 28)
    colors = ["trim_orange", "cable", "nav_white", "warning_red"]
    for k in range(4):
        b = math.radians(70 + 55 * k)
        d = Vector((math.cos(b), math.sin(b), 0))
        start = Vector((x0, y0, -0.04)) + d * 0.05
        mid = Vector((x0, y0, 0.12)) + d * 0.18
        end = Vector((x0, y0, 0.23)) + d * (0.30 + 0.05 * k)
        p.polyline_tube([start, mid, end], 0.012, colors[k], 6)
        p.sphere(0.022, "spark", T(*end), 8, 4)
    p.cylinder(0.035, 0.03, "metal", T(-0.52, 0.52, 0.035), 12)
    p.cylinder(0.035, 0.03, "metal", T(0.52, 0.52, 0.035), 12)
    p.cylinder(0.035, 0.03, "metal", T(-0.52, -0.52, 0.035), 12)
    p.cylinder(0.02, 0.02, "hull_dark", T(0.52, -0.52, 0.025), 10)
    p.box_span((-0.25, 0.42, 0.02), (0.25, 0.47, 0.035), "hull_dark")
    p.box((0.06, 0.04, 0.02), "spark", T(-0.32, 0.445, 0.035))
    p.box_span((-0.45, 0.1, 0.02), (-0.15, 0.25, 0.024), "nav_white")
    for k in range(2):
        x = -0.3 + 0.2 * k
        p.cylinder(0.055, 0.04, "hull_dark", T(x, -0.38, 0.04), 16)
    p.cylinder(0.055, 0.04, "scorched", T(0.1 + 0.15, -0.45, 0.04) @ R("Y", 25), 16)
    p.finish(scene)


def build_floodlight(scene):
    f = Part("floodlight")
    f.box_span((-0.16, 0.0, -0.16), (0.16, 0.06, 0.16), "hull_dark", bevel=0.015)
    f.cylinder(0.06, 0.08, "metal", T(0, 0.1, 0) @ R("X", 90), 12)
    f.tube((0, 0.1, 0), (0, 1.05, 0), 0.04, "metal", 12)
    f.tube((0, 1.0, 0), (0, 1.1, 0), 0.055, "hull_dark", 12)
    f.tube((0, 1.1, 0), (0, 1.75, 0), 0.03, "metal", 12)
    f.box_span((-0.24, 1.73, -0.04), (0.24, 1.77, 0.04), "metal", bevel=0.01)
    for x in (-0.23, 0.23):
        f.box_span((x - 0.015, 1.75, -0.04), (x + 0.015, 2.0, 0.04), "metal")
    head = T(0, 1.95, -0.02) @ R("X", -18)
    f.box((0.42, 0.22, 0.26), "hull_dark", head, bevel=0.02)
    f.box((0.37, 0.17, 0.01), "lamp_lens", head @ T(0, 0, -0.131))
    f.box_span((-0.2, -0.115, -0.15), (0.2, 0.115, -0.13), "metal", head)
    for k in range(6):
        f.box((0.36, 0.008, 0.06), "metal", head @ T(0, -0.085 + 0.034 * k, 0.155))
    f.cylinder(0.022, 0.46, "metal", T(0, 1.95, -0.02) @ R("Y", 90), 10)
    f.tube((0.05, 1.3, 0.03), (0.12, 1.9, 0.12), 0.012, "cable", 5)
    f.finish(scene)


def build_warning_light(scene):
    w = Part("warning_light")
    up = frame((0, 0, 0), (0, 1, 0))
    w.lathe([(0.18, -0.05), (0.18, -0.02), (0.15, 0.0)], "hull_dark", up, 24, cap_start=True)
    w.lathe([(0.13, 0.0), (0.13, 0.08)], "metal", up, 24)
    w.lathe([(0.11, 0.08), (0.11, 0.17), (0.095, 0.22), (0.06, 0.255), (0.0001, 0.27)], "warning_red", up, 24)
    for k in range(4):
        b = math.radians(45 + 90 * k)
        x, z = math.cos(b) * 0.135, math.sin(b) * 0.135
        w.polyline_tube([(x, 0.06, z), (x, 0.22, z), (x * 0.5, 0.285, z * 0.5)], 0.008, "metal", 5)
    w.torus(0.135, 0.008, "metal", up @ T(0, 0, 0.17), 24, 5)
    w.finish(scene)


def build_repair_tool(scene):
    t = Part("repair_tool")
    t.box_span((-0.036, -0.04, -0.24), (0.036, 0.05, 0.07), "hull_dark", bevel=0.01)
    t.box((0.05, 0.135, 0.055), "tool_grip", T(0, -0.10, 0.035) @ R("X", -14), bevel=0.012)
    t.box((0.02, 0.04, 0.02), "trim_orange", T(0, -0.055, -0.01))
    t.box_span((-0.026, 0.05, -0.04), (0.026, 0.055, 0.05), "engine_glow")
    t.box_span((-0.03, 0.045, -0.05), (0.03, 0.052, -0.04), "metal")
    t.lathe([(0.03, -0.24), (0.03, -0.27), (0.02, -0.285)][::-1], "metal", segments=14)
    t.cylinder(0.012, 0.07, "metal", T(0, 0, -0.315), 10)
    t.cylinder(0.019, 0.025, "metal", T(0, 0, -0.34), 6)
    t.torus(0.02, 0.005, "handrail", T(0, 0.0, 0.085) @ R("Y", 90), 12, 4)
    t.box_span((-0.04, -0.03, -0.18), (0.04, 0.03, -0.12), "trim_orange", bevel=0.005)
    t.finish(scene)


def build_rock(scene, name, radii, seed, mat, target):
    r = Part(name, sharp_angle=math.radians(180))
    r.add_bmesh(rock_bmesh(radii, seed), mat, tile=1.2)
    r.fit_corner_radius(target)
    r.finish(scene)


def build_debris_panel(scene):
    p = Part("debris_panel")
    left = [(-0.95, -0.7), (-0.1, -0.7), (-0.1, 0.7), (-0.95, 0.7)]
    right = [(0.0, -0.7), (0.62, -0.7), (0.78, -0.42), (0.66, -0.2), (0.92, 0.05), (0.74, 0.3), (0.86, 0.55), (0.6, 0.7), (0.0, 0.7)]
    flat = columns_matrix((1, 0, 0), (0, 0, -1), (0, 1, 0))
    p.ngon_plate(left, 0.015, "solar_cells", T(0, 0.0, 0) @ flat, tile=2.0)
    p.ngon_plate(left, 0.01, "gold_foil", T(0, -0.01, 0) @ flat, tile=1.0)
    bend = T(-0.05, 0.0, 0) @ R("Z", 22) @ T(0.05, 0, 0)
    p.ngon_plate(right, 0.015, "solar_cells", bend @ flat, tile=2.0)
    p.ngon_plate(right, 0.01, "gold_foil", bend @ T(0, -0.01, 0) @ flat, tile=1.0)
    p.box_span((-1.0, -0.03, -0.72), (-0.92, 0.04, 0.72), "metal")
    p.box_span((-1.0, -0.03, -0.75), (0.0, 0.04, -0.69), "metal")
    p.tube((-1.0, 0.0, 0.0), (-1.0, 0.05, -0.2), 0.03, "scorched", 6)
    p.tube((-0.96, 0.02, 0.4), (-0.6, 0.22, 0.5), 0.015, "truss", 5)
    p.tube((-0.6, 0.22, 0.5), (-0.5, 0.24, 0.62), 0.015, "scorched", 5)
    p.lathe([(0.25, 0.017), (0.12, 0.02)], "scorched", T(-0.5, 0.0, -0.2) @ R("X", -90), 16)
    p.fit_corner_radius(1.3138)
    p.finish(scene)


def build_debris_tank(scene):
    t = Part("debris_tank")
    along = R("Y", 90)
    prof = [(0.06, -1.42), (0.25, -1.38), (0.42, -1.27), (0.53, -1.12), (0.58, -0.9), (0.58, 0.9), (0.53, 1.12), (0.42, 1.27), (0.25, 1.38), (0.06, 1.42)]
    t.lathe(prof, "scrap", along @ T(0, 0, 0.1), 32, cap_start=True, cap_end=True, tile=1.0)
    t.lathe([(0.585, -0.6), (0.585, 0.6)], "gold_foil", along @ T(0, 0, 0.1), 32, arc=230, start_angle=60, tile=0.8)
    t.lathe([(0.59, 0.85), (0.59, 1.0)], "trim_orange", along @ T(0, 0, 0.1), 32)
    t.cylinder(0.08, 0.2, "metal", T(1.6, 0, 0) @ along, 12)
    t.polyline_tube([(1.7, 0, 0), (1.78, 0.05, 0.0), (1.82, 0.18, 0.08)], 0.025, "metal", 6)
    t.lathe([(0.6, -0.04), (0.64, -0.03), (0.64, 0.03), (0.6, 0.04)], "metal", along @ T(0, 0, -0.5), 32, arc=250, start_angle=-30)
    t.box((0.08, 0.35, 0.06), "metal", T(-0.4, -0.72, 0.1) @ R("Z", 25), bevel=0.01)
    t.box((0.08, 0.25, 0.06), "scorched", T(-0.42, -0.95, 0.18) @ R("Z", 55), bevel=0.01)
    t.fit_corner_radius(2.0201)
    t.finish(scene)


def build_debris_beam(scene):
    b = Part("debris_beam")
    half = 0.22
    corners = [(half, half), (half, -half), (-half, -half), (-half, half)]
    length = 2.5
    x0 = -1.5
    for y, z in corners:
        b.tube((x0, y, z), (x0 + length * (0.9 if y > 0 else 1.0), y, z), 0.035, "truss", 6)
    bays = 6
    for k in range(bays + 1):
        x = x0 + length * k / bays
        for (ya, za), (yb, zb) in zip(corners, corners[1:] + corners[:1]):
            if k == bays and ya > 0:
                continue
            b.tube((x, ya, za), (x, yb, zb), 0.02, "truss", 5, caps=False)
        if k < bays:
            x1 = x0 + length * (k + 1) / bays
            b.tube((x, half, half), (x1, half, -half), 0.015, "truss", 4, caps=False)
            b.tube((x, -half, half), (x1, -half, -half), 0.015, "truss", 4, caps=False)
    for y, z, dy, dz in ((half, half, 0.25, 0.1), (half, -half, 0.35, -0.15)):
        b.polyline_tube([(x0 + length * 0.9, y, z), (x0 + length * 0.9 + 0.2, y + dy, z + dz)], 0.035, "scorched", 6)
    b.ngon_plate([(-0.5, 0.0), (0.4, 0.0), (0.55, 0.3), (0.2, 0.55), (-0.3, 0.42)], 0.01, "gold_foil", T(0.2, half + 0.04, 0.0) @ R("X", -80), tile=0.8)
    b.polyline_tube([(0.8, -half, 0.0), (1.05, -0.35, 0.2), (1.2, -0.3, 0.45)], 0.02, "cable", 6)
    b.box_span((-1.6, -0.3, -0.3), (-1.45, 0.3, 0.3), "hull_dark", bevel=0.02)
    b.fit_corner_radius(1.6898)
    b.finish(scene)


BUILDERS = {
    "pod": build_pod,
    "station_hub": build_station_hub,
    "station_ring": build_station_ring,
    "station_solar": build_station_solar,
    "docking_arm": build_docking_arm,
    "docking_port": build_docking_port,
    "docking_clamp": build_docking_clamp,
    "eva_deck": build_eva_deck,
    "hull_panel": build_hull_panel,
    "repair_panel_fixed": build_repair_panel_fixed,
    "repair_panel_damaged": build_repair_panel_damaged,
    "floodlight": build_floodlight,
    "warning_light": build_warning_light,
    "repair_tool": build_repair_tool,
    "debris_rock_a": lambda scene: build_rock(scene, "debris_rock_a", (1.0, 0.82, 1.18), 3, "rock", 1.8356),
    "debris_rock_b": lambda scene: build_rock(scene, "debris_rock_b", (1.38, 0.96, 0.92), 8, "rock_dark", 1.9294),
    "debris_panel": build_debris_panel,
    "debris_tank": build_debris_tank,
    "debris_beam": build_debris_beam,
}


def main():
    names = sys.argv[sys.argv.index("--") + 1:]
    define_materials()
    for name in names:
        scene = bpy.data.scenes[name]
        clear_scene(scene)
        BUILDERS[name](scene)
        print("BUILT", name)
    for material_ in list(bpy.data.materials):
        if material_.users == 0:
            bpy.data.materials.remove(material_)
    for mesh in list(bpy.data.meshes):
        if mesh.users == 0:
            bpy.data.meshes.remove(mesh)
    bpy.ops.file.make_paths_relative()
    bpy.ops.wm.save_mainfile()


main()
