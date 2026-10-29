# Demo video script (target 4:30, allowed 3–6 min)

Record on the Mac with **QuickTime → File → New Screen Recording** (or OBS), window 1280×720, export 1080p.
Start the program with `./build/CozyRoom --novsync` only if you want to show the uncapped FPS; otherwise normal
start is fine. **The FPS counter (top-right overlay + window title) must be visible the whole time** — do not
hide it, do not speed up the footage. Voice-over (or captions) names every feature as it appears.
Each member can record the voice-over for their own part.

| Time | Shot (what you do) | Voice-over / caption | Shows |
|---|---|---|---|
| 0:00–0:20 | Program starts at 9:00, overview. Slowly orbit with the mouse. | "Cozy Room Diorama — a cut-away lo-fi room in C++ and OpenGL 3.3 core. FPS counter top right." | R1, R7 |
| 0:20–0:50 | Press **X** (wireframe) for 3 s, then X again. Orbit around the bed, desk, shelf. | "Over 40 objects built from our own primitives — boxes, spheres, cylinders, cones, a torus — each a VAO with VBO and EBO, drawn with our own shaders." | R1, R2 |
| 0:50–1:20 | Press **2** (window preset). Press **L** for the desk lamp, hold **← →** and **↑ ↓** to aim it. | "Scene graph: the desk lamp is a chain base → shoulder → elbow → head. Rotating the joints moves the spotlight, because its position comes from the head's world matrix." | R3, R4, R6 |
| 1:20–1:35 | Press **4** (record corner): clock and turning record. Press **M** twice. | "The wall clock hands are children of the clock and show the in-game time; the record spins and the tonearm swings in." | R3 |
| 1:35–2:05 | Zoom in on floor/rug/bed textures, then the wallpaper. | "Eleven textures with mipmaps and anisotropic filtering. Walls use world-space UVs so the wallpaper continues across wall pieces without seams." | R5 |
| 2:05–2:40 | Back to **2**. Drag the Time slider 10:00 → 13:00 → 16:00. | "The sun moves; shadow mapping makes the sunlight enter only through the window — you see the window frame on the floor." Toggle **G** off/on: "shadows off… and on." | Stretch 3, R4 |
| 2:40–3:05 | Close to the beam, toggle **B** dust off/on, **V** light shafts off/on. | "Six thousand dust particles in one instanced draw call. Each samples the shadow map, so dust only sparkles inside the sunbeam." | Stretch 1, B4 |
| 3:05–3:30 | Time 17:30 golden hour, press **T** twice. | "Colours change with time of day: warm sunrise, neutral noon, golden hour, cold blue night. T switches to neutral light to compare." | Stretch 2 |
| 3:30–3:50 | Press **C** (fly camera), WASD walk to the bed, mouse look, Esc. | "Orbit and first-person camera, keyboard and mouse, frame-rate independent." | R6, C1 |
| 3:50–4:05 | Show the ImGui panel and press **H** (help). | "All interactions are in the panel; H shows every key." | C3 |
| 4:05–4:30 | **Wow moment:** press **1**, set time 16:30, press **P** (time-lapse, speed 1.5). Let it run to night: the sun sets, the room turns blue, fairy lights, floor lamp and desk lamp switch on one by one. | "And finally — a full day in thirty seconds." (end on the lit room at night) | Wow |

Before exporting check the list: R1 ✓ R2 ✓ R3 ✓ R4 ✓ R5 ✓ R6 ✓ R7 (FPS always visible) ✓, stretch goals 1–3 ✓.
