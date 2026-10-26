# Orbital Dock

A desktop 3D space-station docking and repair simulator built in C++17 and OpenGL 3.3 for the CGVR mini project (CS373IA), RV College of Engineering.

Fly a maintenance pod through a drifting debris field, dock with a slowly rotating space station by matching its position, speed, alignment and roll, then step out on a first-person spacewalk and repair a damaged hull panel before life support runs out.

## Demo video

`media/orbital_dock_playthrough.mp4` is a 2-minute recording of the main menu, the flight through the debris field and the docking approach up to clamp lock (software-rendered on Linux, no audio).

## Build and run

Requirements: CMake 3.20+, a C++17 compiler and an internet connection for the first configure (GLFW, GLM, Assimp and Dear ImGui are downloaded by `FetchContent`).

macOS (Xcode command line tools):

```
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/orbital_dock
```

Windows (Visual Studio 2022 with the C++ workload):

```
cmake -S . -B build
cmake --build build --config Release
build\Release\orbital_dock.exe
```

Linux needs the X11 and audio headers first: `sudo apt install libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev libgl-dev libasound2-dev`.

The `assets/` folder is copied next to the executable after every build, so the game can be started from any working directory.

## How to play

| Phase | Goal |
| --- | --- |
| Flight | Reach the station. Debris drifts through the corridor and some pieces head straight for you. Every hit shakes the camera and costs hull integrity. |
| Docking | Inside 90 m the docking arm unfolds. Line up with the green axis guide, match the station's roll and slide in gently. Distance, relative speed, alignment and roll must all be within tolerance for one second to lock. |
| EVA | Walk along the deck on top of the station to the sparking panel at the far end and hold `E` to repair it before your oxygen runs out. |

### Controls

| Key | Flight and docking | EVA |
| --- | --- | --- |
| `W` / `S` | Forward / reverse thrust | Walk forward / back |
| `A` / `D` | Yaw left / right | Strafe left / right |
| `Q` / `E` | Roll left / right | `E` (hold): repair |
| `R` / `F` | Thrust up / down | |
| Mouse | Pitch and yaw | Look around |
| `Space` | Brake (kills velocity and spin) | |
| `Tab` | Toggle alignment assist (axis guide, port markers, roll matching) | |
| `V` | Chase / cockpit camera | |
| `Esc` | Pause menu | Pause menu |
| `F1` | Debug overlay: FPS, collision volumes | Debug overlay |
| `F2` | With the debug overlay open: jump to the docking approach | |
| `F4` | With the debug overlay open: simulate a debris hit | |
| `Shift` | Fine thrust (25%) for the final approach | |
| `F3` | Side-by-side stereo view | Side-by-side stereo view |
| `F11` | Fullscreen | Fullscreen |

## How the lab requirements are met

| Requirement | Where |
| --- | --- |
| Immersive, interactive, imaginative | First-person EVA, chase and cockpit cameras, screen shake, damage flash, ambient hum and alarms, a three-phase mission with win and lose conditions |
| 3D modelling | Station hub, ring, solar arrays, articulated docking arm, port, clamps, pod, five debris variants, EVA deck, hull panels, floodlights and repair tool in `assets/models` (OBJ + MTL) loaded with Assimp |
| Animation and 3D transformations | Scene graph (`src/scene/SceneNode`) with the rotating station as the root; the arm, port and clamps animate as children and inherit its motion; the pod is reparented under the port on docking; debris translate and tumble |
| Light source | Blinn-Phong shading with a directional sun and up to eight point lights: deck floodlights, a blinking red warning beacon, the green port guide light, the pod headlight and the repair tool's arc (`assets/shaders/blinn_phong.frag`) |
| Sound | miniaudio: thruster loop, impact, hull alarm, docking clunk, lock tones, welding, repair chime and ambient station hum |

## Project layout

```
assets/       models (OBJ/MTL), textures (PNG), audio (WAV), shaders (GLSL 330)
src/core      window, input, timer, state machine, Game (main loop and mission flow)
src/render    shader, mesh, texture, cubemap, model loader, camera, lights, renderer
src/scene     SceneNode transform hierarchy
src/physics   AABB, sphere, collision tests, docking tolerance check
src/gameplay  pod, station, debris, collision resolver, chase camera, EVA controller, mission rules
src/audio     AudioEngine wrapper over miniaudio
src/ui        ImGui layer, HUD, menus
src/platform  the only OS-specific code (executable path lookup)
third_party   GLAD, stb_image, miniaudio
```

## Replacing the placeholder models

The models in `assets/models` are simple generated placeholders. `blender/orbital_dock.blend` holds all of them, one Blender scene per model, at the exact origin, scale and axes the game uses. Model over the placeholder in its scene, save, then export every scene back to `assets/models` with:

```
/Applications/Blender.app/Contents/MacOS/Blender blender/orbital_dock.blend --background --python tools/blender/export_models.py
```

(on Windows use the path to `blender.exe`; the script can also be run from Blender's Scripting tab). The script applies the right OBJ settings and fixes the `.mtl` values the game reads; the top of `tools/blender/export_models.py` explains which Blender material settings map to which values. Textures must live in `assets/textures`. Scenes whose names start with `_` are not exported. Keep each model's origin and scale matching the placeholder, because the station layout, colliders and docking offset in `src/gameplay/Station.cpp` are built around them. `tools/blender/build_reference_blend.py` rebuilds the `.blend` from the OBJ files; it refuses to overwrite an existing `.blend` unless given `-- --force`.
