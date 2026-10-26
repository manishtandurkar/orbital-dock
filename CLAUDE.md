# Orbital Dock

CGVR mini project for Computer Graphics and Virtual Reality (CS373IA), Semester VII, RV College of Engineering. A desktop 3D space-station docking and repair simulator.

## Project Context

- Deliverable: mini project (lab component)
- Team: 2 members
- Timeline: 2 months (8 weeks)
- Hardware: one Mac and one Windows laptop, so everything must build and run natively on both
- No VR headset is available or required. The project is a desktop 3D simulation controlled with keyboard and mouse
- Tech stack is free to choose as long as it works on both macOS and Windows

## Lab Requirements (mini project must have all of these)

1. The 3 I's: Immersive, Interactive, and Imaginative
2. Modelling of 3D objects
3. Animation and 3D transformations should be shown
4. A light source should be used. Sound effects, if used, add value

## Concept

The player controls a small docking pod on screen. The pod flies through a drifting debris field toward a slowly rotating space station, docks at the station's docking port by matching position, speed, and orientation, and then the view switches to a first-person spacewalk (EVA) where the player walks or floats to a damaged hull panel and repairs it before life support runs out.

Game states: `Menu -> Flight -> Docking -> EVA -> Win | Lose`.

### Part 1: Flying to the station

- Keyboard thrusters: forward, back, yaw, roll, up, down, brake
- Debris (rocks and junk) floats around, and some pieces drift toward the player
- Hitting debris triggers an alarm, screen shake, and hull damage
- Goal: reach the station without taking too much damage

### Part 2: Docking

- The station rotates slowly like a real rotating station
- The player must match the station's motion and line up with the docking port axis, then slide in gently
- When distance, relative velocity, and alignment angle are all within tolerance for a short hold time, the pod locks onto the port

### Part 3: Repair walk (EVA)

- After docking the camera switches to first person, as if standing outside on the hull
- The player moves to a broken panel and presses a key to repair it
- Station floodlights and blinking warning lights illuminate the scene, with ambient station sounds and alarms
- A life-support timer counts down; failing to repair in time loses the game

## Tech Stack

- Language: C++17
- Graphics: OpenGL 3.3 core profile (GLSL `#version 330 core`). Do not use anything above 3.3; macOS caps at 4.1 and deprecates OpenGL
- Windowing/input: GLFW
- GL loader: GLAD (vendored in `third_party/glad`)
- Math: GLM
- Model loading: Assimp (OBJ + MTL only, importers trimmed to keep build fast)
- Textures: stb_image (vendored single header)
- Audio: miniaudio (vendored single header)
- HUD/menus: Dear ImGui (GLFW + OpenGL3 backends)
- Build: CMake >= 3.20, dependencies fetched with `FetchContent`
- Modelling: Blender, exported as OBJ with MTL

Nothing platform-specific is allowed in game code. Any OS difference lives in `CMakeLists.txt` or a single `src/platform/` file.

Fallback if the C++/OpenGL setup becomes too costly: Three.js + Cannon-es + Web Audio in the browser.

## Build

macOS:

```
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/orbital_dock
```

Windows (Visual Studio generator):

```
cmake -S . -B build
cmake --build build --config Release
build\Release\orbital_dock.exe
```

Assets are copied next to the executable by CMake; always load assets via paths relative to the executable, never absolute paths.

Cross-platform gotchas to respect:

- Set `GLFW_OPENGL_FORWARD_COMPAT` and core profile hints before window creation (required on macOS)
- Use `glfwGetFramebufferSize` for `glViewport`, not window size (Retina scaling)
- Keep line endings LF; `.gitattributes` enforces this
- Use `std::filesystem` or forward slashes for paths

## Project Layout

```
CMakeLists.txt
CLAUDE.md
assets/
  models/        OBJ + MTL exported from Blender
  textures/
  audio/
  shaders/       *.vert, *.frag
src/
  main.cpp
  core/          Window, Input, Timer, Game state machine
  render/        Shader, Mesh, Model, Camera, Light, Renderer
  scene/         SceneNode (transform hierarchy), Entity
  physics/       AABB, Sphere, collision tests, docking check
  gameplay/      Pod, Station, Debris, EVA controller, mission rules
  audio/         AudioEngine wrapper over miniaudio
  ui/            HUD and menus (ImGui)
third_party/
  glad/ stb/ miniaudio/
```

## Assets to Model (Blender)

- Space station: central hub, rotating ring or modules, docking arm, docking port, hull panels including one repairable panel
- Docking pod
- Debris: several rock and junk variants
- EVA props: repair tool, panel details
- Optional skybox/starfield texture

## Architecture Notes

- `SceneNode` holds local transform (position, rotation quaternion, scale), parent pointer, and children. World matrix = parent world * local. The station rotates as a root node; the docking arm and port are children, so port motion is inherited
- On a successful dock the pod is reparented under the docking port node (snap)
- Collision uses AABB for station parts and bounding spheres for debris and the pod. Resolve impacts with a simple impulse and trigger screen shake, sound, and hull damage
- Docking succeeds when relative distance, relative velocity, and alignment angle to the port axis are all within tolerance for a short hold time
- Debris behaviour: each piece has velocity and angular velocity, with a simple state (drifting, tumbling after impact, despawned out of range)
- Lighting: Blinn-Phong. One directional light (sun), several point lights (station floodlights, blinking warning light). Light parameters live in a `Light` struct passed as uniforms
- Cameras: free-fly/chase camera in Flight and Docking, first-person camera in EVA. Both use the same `Camera` class with different controllers
- Depth buffer and back-face culling enabled
- Audio events: thruster loop, docking clunk, impact, hull alarm, ambient hum, repair completion
- Stretch goal only after everything else works: side-by-side stereo rendering using two offset cameras

## Controls (initial draft)

- Flight: W/S forward/back thrust, A/D yaw, Q/E roll, R/F up/down, mouse look, Space brake
- Docking: same, plus `Tab` toggles alignment assist overlay
- EVA: WASD move, mouse look, `E` interact
- Global: `Esc` pause/menu, `F1` toggle debug overlay (collision volumes, FPS)

## Code Conventions

- C++ only
- K&R brace style: opening brace on the same line
- Braces on every statement, including single-line `if`, `else`, `for`, and `while` bodies
- No inline comments. Names must carry the meaning; extract a well-named function instead of commenting
- Prefer `const`, references, and RAII. No raw `new`/`delete`; use `std::unique_ptr` or value types
- One class per header/source pair, `PascalCase` types, `camelCase` functions and variables, `kConstant` for constants
- Shaders live in `assets/shaders`; never inline GLSL in C++
- Check GL errors in debug builds through one helper, not scattered calls

## Workflow

- Branch per feature, small commits, pull before starting work, never force-push `main`
- Both platforms must build before merging to `main`. If a change only compiles on one OS, fix it before merging
- Do not add dependencies without discussing; every new dependency must build via CMake on macOS and Windows

## Milestones (8 weeks)

1. Weeks 1-2, engine skeleton: CMake cross-platform setup, window and input via GLFW, camera system, basic VAO/VBO rendering pipeline, shader loader
2. Weeks 3-4, modelling and lighting: build station, pod, debris, and tools in Blender; import via Assimp; Blinn-Phong lighting with a dynamic point light
3. Week 5, collision and HUD: AABB/sphere collision for docking alignment and debris impacts; HUD and menus for alignment indicators and mission status
4. Week 6, animation and behaviour: hierarchical transforms for the rotating station and articulating docking arm; autonomous debris drift
5. Week 7, audio and polish: miniaudio integration for docking clunk, alarms, ambient hum; mission flow (flight, dock, repair, escape); optional stereo view
6. Week 8, cross-platform QA and demo: test on both Mac and Windows, fix platform quirks, record the demo video, write the report

## Team Split

- Member A (rendering): pipeline, shaders, lighting, camera and projections, model import, visible-surface handling
- Member B (interaction/physics): input, collision, docking logic, hierarchy/kinematics, debris behaviour, audio, HUD
- Shared: mission flow, integration, testing on both OSes, report
- Converge weekly to integrate

## Definition of Done

- Builds and runs from a clean clone on both macOS and Windows with the commands above
- Runs at a stable 60 FPS on a laptop integrated GPU
- All four lab requirements demonstrable within a 3-minute demo run

## Instructions for Claude

- Follow the code conventions above exactly, including no inline comments and braces on every statement
- Keep game code free of OS-specific calls; flag it if a task seems to require one
- When adding a file, update `CMakeLists.txt` and verify it still configures for both platforms
- Prefer small, runnable increments: after each change the project should still build and launch
- Ask before introducing a new dependency or changing the directory layout

## Progress Log

@PROGRESS.md
