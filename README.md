# Cozy Room Diorama

A small cut-away "lo-fi" room that lives through a full day — real-time 3D in **C++ and modern OpenGL 3.3 core**.
Final project for *Computer Graphics Fundamentals* (topic 2, ★).

The sun moves across the sky and its beam travels through the window across the floor, dust sparkles in the light,
the colours shift from orange sunrise to neutral noon, golden hour and a cold blue night, and at dusk the fairy
lights, floor lamp and desk lamp switch on one by one.

![Golden hour](docs/screenshots/02_golden_hour.png)

| Morning | Night |
|---|---|
| ![](docs/screenshots/01_overview_morning.png) | ![](docs/screenshots/03_night_overview.png) |
| ![](docs/screenshots/04_window_beam.png) | ![](docs/screenshots/05_bed_fairy_lights.png) |

## Team

| Member | Main responsibility (code) |
|---|---|
| Rinatuly Bakdaulet (lead) | Application core and build: `App`, `Camera`, `SceneNode` hierarchy, `ShadowMap`, CMake, README |
| Zhumagali Yessen | Lighting: `scene.vert/frag` (Blinn-Phong, point/spot lights, PCF), `DayCycle`, `Lights` |
| Abdimutalip Aidana | Modelling & texturing: `Primitives`, `SceneLayout`, `Texture`, `tools/gen_textures.py`, `sky.frag` |
| Zhagypar Aruzhan | Effects & UI: `DustParticles`, `PostProcess` (bloom, light shafts, tone mapping), `UI`, FPS testing |

Groups: *(fill in)*. Each member also wrote the matching report section and presents their part.

## Build and run

Everything the program needs (GLFW, GLAD, GLM, stb_image, Dear ImGui) is in `external/` and is compiled together
with the project — nothing has to be installed except a C++17 compiler and CMake ≥ 3.16.

### macOS (VS Code)

```bash
xcode-select --install          # once, if the compiler is not installed yet
brew install cmake              # once, if CMake is not installed yet
git clone <repo-url> cozy-room-diorama && cd cozy-room-diorama
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/CozyRoom
```

In VS Code: *Terminal → Run Build Task* (⇧⌘B) runs the same commands (see `.vscode/tasks.json`); with the
*CMake Tools* extension you can also just press *Build* and *Run* in the status bar.

### Windows (Visual Studio 2019+ / VS Code)

```bat
cmake -S . -B build
cmake --build build --config Release
build\Release\CozyRoom.exe
```

### Linux

Install the X11 development headers once (`sudo apt install libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev libgl-dev`), then use the same commands as on macOS.

The program finds `shaders/` and `assets/` through the path baked in by CMake, so it can be started from any
folder. Optional flags: `--novsync` (measure the real frame rate), `--hour 18.5`, `--preset 0..3`, `--lights`,
`--capture out.png --frames 60` (render and save one image, used for the report figures).

## Controls

An on-screen panel (Dear ImGui) has every setting; press **H** for the help window.

| Key | Action |
|---|---|
| Mouse drag / wheel | orbit the camera / zoom |
| W A S D, Q E (+Shift) | orbit and zoom (Orbit mode), move (Fly mode) |
| C | switch Orbit ↔ Fly (FPS) camera, Esc releases the mouse |
| 1 2 3 4 | camera presets: overview, window, bed, record corner |
| [ ] | time of day − / + |
| P | play / pause the day time-lapse |
| L / K / F | desk lamp / floor lamp / fairy lights |
| N | automatic lights at dusk on/off |
| ← → ↑ ↓ | aim the desk lamp (rotates two joints of its hierarchy, the spotlight follows) |
| M | record player play / stop |
| G / B / V | shadows / dust / light shafts |
| T | warm–cold colour grading on/off (compare with neutral light) |
| X | wireframe |
| F12 | save a screenshot to `screenshots/` |
| Tab / H / Esc | hide panel / help / quit |

## Features and requirements

| # | Requirement | How it is met |
|---|---|---|
| R1 | Modern pipeline | OpenGL 3.3 core (forward-compatible on macOS), every mesh is a VAO + VBO + EBO (`Mesh`), all shaders are our own `.vert/.frag` files in `shaders/`, no fixed-function calls |
| R2 | ≥ 5 objects | 40+ distinct objects built from our own primitives (`Primitives.cpp`): bed, desk, chair, laptop, desk lamp, floor lamp, bookshelf with 90+ books, record player, speaker, wall clock, plants, rug, pouf, poster, curtains, window… |
| R3 | Transformations + hierarchy | Model/view/projection built with GLM; scene graph (`SceneNode`, world = parent × T·R·S). Parent–child chains: desk lamp (base → yaw → shoulder → elbow → head → shade/bulb), wall clock (body → hour/minute hands showing the in-game time), record player (base → platter → vinyl, tonearm), laptop (base → hinge → lid → screen), globe |
| R4 | Lighting | Per-fragment Blinn-Phong (ambient + diffuse + specular) with 1 directional light (sun/moon, moves with time), 1 spotlight (desk lamp, toggle + movable), up to 23 point lights (floor lamp, 14 fairy lights, laptop glow) and hemispheric ambient |
| R5 | Texturing | 11 textures with mipmaps (trilinear + anisotropic), sRGB colour space, correct UVs; walls and floor use world-space UVs so the pattern continues across wall pieces without stretching or seams |
| R6 | Interaction | Orbit and FPS camera (keyboard + mouse, frame-rate independent smoothing) plus: time-of-day slider and time-lapse, 3 switchable lights + auto mode, aimable desk lamp, record player, effect toggles, camera presets |
| R7 | Performance | FPS in the window title and in the overlay; instancing for books, fairy bulbs and 6 000 dust particles; all buffers are static (no per-frame uploads) |
| R8 | Build | CMake, all libraries vendored, steps above |

**Stretch goals from the topic (all three implemented):**

1. **Dust particles in the light beam** — 6 000 instanced billboards animated in the vertex shader; each particle
   samples the sun's shadow map, so dust only sparkles inside the beam (`DustParticles`, `dust.vert`).
2. **Warm/cold colour change by time of day** — `DayCycle` interpolates sky, ambient, sun colour and a final
   colour grade from keyframes (sunrise, noon, golden hour, dusk, night); **T** toggles it off for comparison.
3. **Shadow mapping for the sun** — 2048² depth map with a tightly fitted orthographic light camera, slope-scaled
   bias and 3×3 hardware PCF. The diorama has no ceiling, so invisible "shadow-only" occluders make sure the sun
   only enters through the window (`ShadowMap`, `scene.frag`).

**Extras:** volumetric light shafts (ray-marching the shadow map), HDR rendering with 4× MSAA, bloom, ACES tone
mapping, procedural sky through the window (sun, moon, stars, hills), all textures generated by our own script.

## Project structure

```
src/            C++ sources (one class per file pair, every file < 400 lines)
  App           window, input, main loop, FPS counter, screenshots
  Camera        orbit + fly camera
  SceneNode     scene graph node and material
  Scene         owns meshes/textures, animates the props, collects lights
  SceneLayout   builds the room furniture
  Primitives    box / plane / quad / sphere / cylinder-cone / torus generators
  Mesh, Shader, Texture        OpenGL wrappers
  DayCycle      sun path and colours by time of day
  Renderer      frame: shadow pass, scene pass, post
  ShadowMap, PostProcess, DustParticles, UI
shaders/        GLSL 330 core shaders
assets/textures generated textures (tools/gen_textures.py)
tools/          texture generator script
docs/           screenshots, demo video script
external/       GLFW 3.4, GLAD 2 (GL 3.3 core), GLM 1.0.1, stb, Dear ImGui 1.91.5
```

## AI Use note

We used **Claude (Anthropic)** as a programming assistant: it produced the first version of the project
structure, the C++/GLSL code, the texture generator script and drafts of this README. We then reviewed, tested
and changed the code ourselves (tuning the light intensities and colours, the lamp geometry, the sky and the dust
amount, fixing issues found while testing on our laptops). Every member studied and can explain the part listed
next to their name in the Team table. Details are in the report's "AI Use Note".

## Credits

See [CREDITS.md](CREDITS.md).
