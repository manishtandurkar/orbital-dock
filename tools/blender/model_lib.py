"""Geometry and material helpers used by build_models.py."""

import math
from pathlib import Path

import bmesh
import bpy
from mathutils import Matrix, Vector, noise

REPO = Path(__file__).resolve().parents[2]
TEXTURES = REPO / "assets" / "textures"
SHARP_ANGLE = math.radians(32)


def material(name, kd=None, ks=None, ns=None, ke=None, texture=None):
    m = bpy.data.materials.get(name)
    created = m is None
    if created:
        m = bpy.data.materials.new(name)
        m.use_nodes = True
    if not created and kd is None and texture is None:
        return m
    tree = m.node_tree
    bsdf = tree.nodes.get("Principled BSDF")
    kd = kd or (0.8, 0.8, 0.8)
    ks = ks or (0.3, 0.3, 0.3)
    ns = ns or 32.0
    ke = ke or (0.0, 0.0, 0.0)
    bsdf.inputs["Base Color"].default_value = (*kd, 1.0)
    level = max(ks)
    bsdf.inputs["Specular IOR Level"].default_value = level
    bsdf.inputs["Specular Tint"].default_value = (*[c / level if level > 0 else 1.0 for c in ks], 1.0)
    bsdf.inputs["Roughness"].default_value = 1.0 - math.sqrt(min(ns, 1000.0) / 1000.0)
    bsdf.inputs["Emission Color"].default_value = (*ke, 1.0)
    bsdf.inputs["Emission Strength"].default_value = 1.0 if max(ke) > 0 else 0.0
    m.diffuse_color = (*kd, 1.0)
    for link in list(bsdf.inputs["Base Color"].links):
        tree.links.remove(link)
    if texture:
        node = next((n for n in tree.nodes if n.type == "TEX_IMAGE"), None) or tree.nodes.new("ShaderNodeTexImage")
        path = TEXTURES / f"{texture}.png"
        image = bpy.data.images.get(path.name) or bpy.data.images.load(str(path))
        image.filepath = str(path)
        image.reload()
        node.image = image
        tree.links.new(node.outputs["Color"], bsdf.inputs["Base Color"])
    return m


def frame(origin, z_dir, up_hint=(0, 1, 0)):
    z = Vector(z_dir).normalized()
    up = Vector(up_hint)
    if abs(z.dot(up.normalized())) > 0.99:
        up = Vector((1, 0, 0))
    x = up.cross(z).normalized()
    y = z.cross(x).normalized()
    m = Matrix((x, y, z)).transposed().to_4x4()
    m.translation = Vector(origin)
    return m


def T(x=0.0, y=0.0, z=0.0):
    return Matrix.Translation((x, y, z))


def R(axis, degrees):
    return Matrix.Rotation(math.radians(degrees), 4, axis)


def S(x, y=None, z=None):
    y = x if y is None else y
    z = x if z is None else z
    return Matrix.Diagonal((x, y, z, 1.0))


class Part:
    def __init__(self, name, sharp_angle=SHARP_ANGLE):
        self.name = name
        self.bm = bmesh.new()
        self.uv = self.bm.loops.layers.uv.new("UVMap")
        self.materials = []
        self.sharp_angle = sharp_angle

    def mat(self, name):
        if name not in self.materials:
            self.materials.append(name)
        return self.materials.index(name)

    def _add_faces(self, verts, faces, uvs, mat, matrix):
        index = self.mat(mat)
        made = [self.bm.verts.new(matrix @ Vector(v)) for v in verts]
        for face, face_uvs in zip(faces, uvs):
            try:
                f = self.bm.faces.new([made[i] for i in face])
            except ValueError:
                continue
            f.material_index = index
            for loop, uv in zip(f.loops, face_uvs):
                loop[self.uv].uv = uv

    def add_bmesh(self, temp, mat, matrix=Matrix(), tile=1.0, uv_mode="box"):
        temp.normal_update()
        verts = [v.co.copy() for v in temp.verts]
        lookup = {v: i for i, v in enumerate(temp.verts)}
        faces, uvs = [], []
        temp_uv = temp.loops.layers.uv.active
        for f in temp.faces:
            faces.append([lookup[v] for v in f.verts])
            if uv_mode == "keep" and temp_uv is not None:
                uvs.append([l[temp_uv].uv.copy() for l in f.loops])
                continue
            n = f.normal
            axis = max(range(3), key=lambda i: abs(n[i]))
            a, b = [(1, 2), (0, 2), (0, 1)][axis]
            world = [matrix @ v.co for v in f.verts]
            uvs.append([(w[a] / tile, w[b] / tile) for w in world])
        self._add_faces(verts, faces, uvs, mat, matrix)
        temp.free()

    def box(self, size, mat, matrix=Matrix(), bevel=0.0, tile=1.0):
        temp = bmesh.new()
        bmesh.ops.create_cube(temp, size=1.0)
        bmesh.ops.scale(temp, vec=Vector(size), verts=temp.verts)
        if bevel > 0:
            bmesh.ops.bevel(temp, geom=list(temp.edges) + list(temp.verts), offset=bevel, segments=1, affect="EDGES", profile=0.5, clamp_overlap=True)
        self.add_bmesh(temp, mat, matrix, tile)

    def box_span(self, lo, hi, mat, matrix=Matrix(), bevel=0.0, tile=1.0):
        lo, hi = Vector(lo), Vector(hi)
        self.box(hi - lo, mat, matrix @ T(*((lo + hi) / 2)), bevel, tile)

    def lathe(self, profile, mat, matrix=Matrix(), segments=32, cap_start=False, cap_end=False, tile=1.0, arc=360.0, start_angle=0.0, u_tiles=None):
        rings = len(profile)
        full = abs(arc - 360.0) < 1e-6
        columns = segments if full else segments + 1
        verts = []
        for r, z in profile:
            for j in range(columns):
                a = math.radians(start_angle + arc * j / segments)
                verts.append((r * math.cos(a), r * math.sin(a), z))
        lengths = [0.0]
        for i in range(1, rings):
            lengths.append(lengths[-1] + math.dist(profile[i], profile[i - 1]))
        max_r = max(r for r, _ in profile)
        span_u = u_tiles if u_tiles is not None else (2 * math.pi * max_r * arc / 360.0) / tile
        faces, uvs = [], []
        for i in range(rings - 1):
            for j in range(segments):
                j1 = (j + 1) % columns
                faces.append([i * columns + j, i * columns + j1, (i + 1) * columns + j1, (i + 1) * columns + j])
                u0, u1 = span_u * j / segments, span_u * (j + 1) / segments
                v0, v1 = lengths[i] / tile, lengths[i + 1] / tile
                uvs.append([(u0, v0), (u1, v0), (u1, v1), (u0, v1)])
        if cap_start and profile[0][0] > 0:
            face = list(range(columns))[::-1][: segments if full else columns]
            faces.append(face)
            uvs.append([(verts[k][0] / tile, verts[k][1] / tile) for k in face])
        if cap_end and profile[-1][0] > 0:
            base = (rings - 1) * columns
            face = [base + k for k in range(segments if full else columns)]
            faces.append(face)
            uvs.append([(verts[k][0] / tile, verts[k][1] / tile) for k in face])
        self._add_faces(verts, faces, uvs, mat, matrix)

    def cylinder(self, radius, length, mat, matrix=Matrix(), segments=24, caps=True, tile=1.0):
        self.lathe([(radius, -length / 2), (radius, length / 2)], mat, matrix, segments, caps, caps, tile)

    def tube(self, p0, p1, radius, mat, segments=10, caps=True, tile=1.0):
        p0, p1 = Vector(p0), Vector(p1)
        d = p1 - p0
        self.cylinder(radius, d.length, mat, frame((p0 + p1) / 2, d), segments, caps, tile)

    def sphere(self, radius, mat, matrix=Matrix(), segments=24, rings=12, tile=1.0):
        profile = [(radius * math.sin(math.pi * i / rings) + 1e-4, -radius * math.cos(math.pi * i / rings)) for i in range(rings + 1)]
        self.lathe(profile, mat, matrix, segments, False, False, tile)

    def torus(self, major, minor, mat, matrix=Matrix(), segments=48, sides=10, tile=1.0):
        profile = [(major + minor * math.cos(2 * math.pi * i / sides), minor * math.sin(2 * math.pi * i / sides)) for i in range(sides + 1)]
        self.lathe(profile, mat, matrix, segments, False, False, tile)

    def polyline_tube(self, points, radius, mat, segments=8):
        for a, b in zip(points, points[1:]):
            self.tube(a, b, radius, mat, segments, caps=False)
        for p in points:
            self.sphere(radius * 1.02, mat, T(*p), segments, max(4, segments // 2))

    def handrail(self, p0, p1, standoff_dir, mat="handrail", height=0.12, radius=0.022):
        p0, p1, up = Vector(p0), Vector(p1), Vector(standoff_dir).normalized()
        a, b = p0 + up * height, p1 + up * height
        self.tube(p0, a, radius * 0.9, mat, 6)
        self.tube(p1, b, radius * 0.9, mat, 6)
        self.tube(a, b, radius, mat, 8)

    def ngon_plate(self, points2d, thickness, mat, matrix=Matrix(), tile=1.0):
        temp = bmesh.new()
        vs = [temp.verts.new((x, y, 0.0)) for x, y in points2d]
        face = temp.faces.new(vs)
        ext = bmesh.ops.extrude_face_region(temp, geom=[face])
        moved = [e for e in ext["geom"] if isinstance(e, bmesh.types.BMVert)]
        bmesh.ops.translate(temp, vec=(0, 0, thickness), verts=moved)
        bmesh.ops.recalc_face_normals(temp, faces=temp.faces)
        self.add_bmesh(temp, mat, matrix, tile)

    def fit_corner_radius(self, target):
        cos = [v.co for v in self.bm.verts]
        lo = Vector([min(c[i] for c in cos) for i in range(3)])
        hi = Vector([max(c[i] for c in cos) for i in range(3)])
        factor = target / max(lo.length, hi.length)
        for v in self.bm.verts:
            v.co *= factor

    def finish(self, scene):
        bm = self.bm
        bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
        for f in bm.faces:
            f.smooth = True
        for e in bm.edges:
            if len(e.link_faces) != 2 or e.calc_face_angle(math.pi) > self.sharp_angle:
                e.smooth = False
        mesh = bpy.data.meshes.new(self.name)
        bm.to_mesh(mesh)
        bm.free()
        for name in self.materials:
            mesh.materials.append(bpy.data.materials[name])
        obj = bpy.data.objects.new(self.name, mesh)
        obj.rotation_euler = (math.pi / 2, 0.0, 0.0)
        scene.collection.objects.link(obj)
        return obj


def clear_scene(scene):
    for obj in list(scene.objects):
        bpy.data.objects.remove(obj)


def columns_matrix(cx, cy, cz):
    return Matrix((cx, cy, cz)).transposed().to_4x4()


def rock_bmesh(radius_xyz, seed, subdivisions=4, crater_count=7, roughness=0.22):
    temp = bmesh.new()
    bmesh.ops.create_icosphere(temp, subdivisions=subdivisions, radius=1.0)
    offset = Vector((seed * 13.1, seed * 7.7, seed * 3.3))
    import random
    rnd = random.Random(seed)
    craters = [(Vector((rnd.uniform(-1, 1), rnd.uniform(-1, 1), rnd.uniform(-1, 1))).normalized(), rnd.uniform(0.18, 0.42)) for _ in range(crater_count)]
    for v in temp.verts:
        d = v.co.normalized()
        h = 1.0 + roughness * noise.fractal(d * 1.3 + offset, 0.8, 2.0, 5)
        h += 0.06 * noise.noise(d * 6.0 + offset)
        for center, size in craters:
            dist = math.acos(max(-1.0, min(1.0, d.dot(center)))) / size
            if dist < 1.0:
                h -= 0.10 * size * (1 - dist * dist) * 2.2
            elif dist < 1.35:
                h += 0.05 * size * math.sin((dist - 1.0) / 0.35 * math.pi) * 2.0
        v.co = Vector((d.x * radius_xyz[0], d.y * radius_xyz[1], d.z * radius_xyz[2])) * h
    return temp
