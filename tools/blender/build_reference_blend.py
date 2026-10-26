"""Build blender/orbital_dock.blend from the OBJ models in assets/models.

Each model gets its own Blender scene named after the OBJ file, sitting at the
same origin, scale and axes the game uses. Model over the placeholder, then run
export_models.py to write the OBJ + MTL files back.

Run from the repository root:
    /Applications/Blender.app/Contents/MacOS/Blender --background --python tools/blender/build_reference_blend.py
Add "-- --force" to replace an existing .blend (this discards work saved in it).
"""

import re
import sys
from pathlib import Path

import bpy

REPO_ROOT = Path(__file__).resolve().parents[2]
MODELS_DIR = REPO_ROOT / "assets" / "models"
BLEND_PATH = REPO_ROOT / "blender" / "orbital_dock.blend"
DUPLICATE_SUFFIX = re.compile(r"\.\d{3}$")


def script_arguments():
    if "--" in sys.argv:
        return sys.argv[sys.argv.index("--") + 1:]
    return []


def merge_duplicate_materials(scene):
    for obj in scene.objects:
        for slot in obj.material_slots:
            material = slot.material
            if material is None or not DUPLICATE_SUFFIX.search(material.name):
                continue
            original = bpy.data.materials.get(DUPLICATE_SUFFIX.sub("", material.name))
            if original is not None:
                slot.material = original


def read_specular_colors(mtl_path):
    colors = {}
    current = None
    for line in mtl_path.read_text().splitlines():
        parts = line.split()
        if not parts:
            continue
        if parts[0] == "newmtl":
            current = parts[1]
        elif parts[0] == "Ks" and current is not None:
            colors[current] = [float(value) for value in parts[1:4]]
    return colors


def keep_specular_color_as_tint(specular_colors):
    for name, color in specular_colors.items():
        material = bpy.data.materials.get(name)
        if material is None or material.node_tree is None:
            continue
        bsdf = material.node_tree.nodes.get("Principled BSDF")
        level = max(color)
        if bsdf is None or level <= 0.0:
            continue
        bsdf.inputs["Specular IOR Level"].default_value = level
        bsdf.inputs["Specular Tint"].default_value = (*[channel / level for channel in color], 1.0)


def remove_unused_materials():
    for material in list(bpy.data.materials):
        if material.users == 0:
            bpy.data.materials.remove(material)


def import_model_into_new_scene(obj_path):
    scene = bpy.data.scenes.new(obj_path.stem)
    with bpy.context.temp_override(scene=scene, view_layer=scene.view_layers[0]):
        bpy.ops.wm.obj_import(filepath=str(obj_path), forward_axis="NEGATIVE_Z", up_axis="Y")
    merge_duplicate_materials(scene)
    keep_specular_color_as_tint(read_specular_colors(obj_path.with_suffix(".mtl")))


def main():
    if BLEND_PATH.exists() and "--force" not in script_arguments():
        print(f"{BLEND_PATH} already exists; pass -- --force to replace it")
        sys.exit(1)

    default_scene = bpy.context.scene
    obj_paths = sorted(MODELS_DIR.glob("*.obj"))
    for obj_path in obj_paths:
        import_model_into_new_scene(obj_path)

    for obj in list(default_scene.objects):
        bpy.data.objects.remove(obj)
    bpy.data.scenes.remove(default_scene)
    remove_unused_materials()

    BLEND_PATH.parent.mkdir(parents=True, exist_ok=True)
    bpy.ops.wm.save_as_mainfile(filepath=str(BLEND_PATH), relative_remap=True)
    print(f"Saved {len(obj_paths)} model scenes to {BLEND_PATH}")


main()
