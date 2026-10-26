"""Export every scene in blender/orbital_dock.blend to assets/models/<scene>.obj + .mtl.

Scenes whose names start with "_" are skipped, so they can hold references or
work in progress. Settings match what the game loads: Forward -Z, Up Y,
triangulated, with normals, UVs and materials.

How the game reads each material (Principled BSDF):
    Kd  Base Color, or for a textured material the Viewport Display color
        (the game multiplies it with the texture; use white for the plain texture)
    Ks  Specular IOR Level x Specular Tint
    Ns  Roughness
    Ke  Emission Color x Emission Strength
Textures must live in assets/textures.

Run from the Blender Scripting tab with the .blend open, or headless:
    /Applications/Blender.app/Contents/MacOS/Blender blender/orbital_dock.blend --background --python tools/blender/export_models.py
Add "-- --out <folder>" to export somewhere else (useful for testing).
"""

import os
import sys
from pathlib import Path

import bpy

TEXTURE_MAP_KEYWORDS = ("map_Kd", "map_Ks", "map_Ke", "map_Bump", "map_d", "map_Ns", "map_refl", "bump", "disp")


def repo_root():
    return Path(bpy.data.filepath).resolve().parents[1]


def script_arguments():
    if "--" in sys.argv:
        return sys.argv[sys.argv.index("--") + 1:]
    return []


def output_directory():
    arguments = script_arguments()
    if "--out" in arguments:
        return Path(arguments[arguments.index("--out") + 1]).resolve()
    return repo_root() / "assets" / "models"


def exportable_scenes():
    return [scene for scene in bpy.data.scenes if not scene.name.startswith("_")]


def export_scene(scene, obj_path):
    with bpy.context.temp_override(scene=scene, view_layer=scene.view_layers[0]):
        bpy.ops.wm.obj_export(
            filepath=str(obj_path),
            forward_axis="NEGATIVE_Z",
            up_axis="Y",
            export_selected_objects=False,
            apply_modifiers=True,
            export_triangulated_mesh=True,
            export_normals=True,
            export_uv=True,
            export_materials=True,
            path_mode="ABSOLUTE",
        )


def format_color(color):
    return " ".join(f"{channel:.6f}" for channel in color[:3])


def principled_bsdf(material):
    if material is None or material.node_tree is None:
        return None
    return material.node_tree.nodes.get("Principled BSDF")


def tinted_specular_line(bsdf):
    level = bsdf.inputs["Specular IOR Level"].default_value
    tint = bsdf.inputs["Specular Tint"].default_value
    return "Ks " + format_color([level * channel for channel in tint[:3]])


def texture_path_for_game(texture_reference, scene_name):
    texture = Path(texture_reference).resolve()
    textures_dir = repo_root() / "assets" / "textures"
    if textures_dir not in texture.parents:
        print(f"WARNING {scene_name}: texture {texture} is outside assets/textures; copy it there and relink it")
    return Path(os.path.relpath(texture, repo_root() / "assets" / "models")).as_posix()


def material_block_for_game(block, scene_name):
    material = bpy.data.materials.get(block[0].split(maxsplit=1)[1])
    bsdf = principled_bsdf(material)
    keywords = [line.split()[0] for line in block if line.split()]
    fixed = []
    for line in block:
        parts = line.split(maxsplit=1)
        if parts and parts[0] == "Ks" and bsdf is not None:
            fixed.append(tinted_specular_line(bsdf))
        elif parts and parts[0] in TEXTURE_MAP_KEYWORDS:
            fixed.append(f"{parts[0]} {texture_path_for_game(parts[1], scene_name)}")
        else:
            fixed.append(line)
    if "Kd" not in keywords and material is not None:
        fixed.insert(1, "Kd " + format_color(material.diffuse_color))
    return fixed


def fix_material_file_for_game(mtl_path, scene_name):
    header = []
    blocks = []
    for line in mtl_path.read_text().splitlines():
        if line.startswith("newmtl "):
            blocks.append([line])
        elif blocks:
            blocks[-1].append(line)
        else:
            header.append(line)
    lines = header + [line for block in blocks for line in material_block_for_game(block, scene_name)]
    mtl_path.write_text("\n".join(lines) + "\n")


def main():
    if not bpy.data.filepath:
        print("Save the .blend inside the repository's blender/ folder first")
        sys.exit(1)
    out_dir = output_directory()
    out_dir.mkdir(parents=True, exist_ok=True)
    scenes = exportable_scenes()
    for scene in scenes:
        obj_path = out_dir / f"{scene.name}.obj"
        export_scene(scene, obj_path)
        fix_material_file_for_game(obj_path.with_suffix(".mtl"), scene.name)
        print(f"Exported {scene.name}")
    print(f"Exported {len(scenes)} models to {out_dir}")


main()
