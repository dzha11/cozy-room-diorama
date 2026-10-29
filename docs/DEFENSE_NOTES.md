# Defense notes — what each member should be able to explain

The instructor may ask anyone about any part, so everyone should read all four sections, and know their own in
depth (open the files and follow the code while reading).

---

## 1. Rinatuly Bakdaulet — application core, camera, hierarchy, shadow mapping

**Frame loop (`App::run`)**: poll events → read input → advance time of day → update camera → `DayCycle::evaluate`
→ `Scene::update` (animations) → `Scene::lights` → `Renderer::render` → ImGui → swap buffers.
`dt` is the real time between frames (clamped to 0.1 s), so everything moves at the same speed at any FPS.

**OpenGL context**: GLFW asks for 3.3 core profile; on macOS `GLFW_OPENGL_FORWARD_COMPAT` is required (macOS only
gives core contexts). GLAD loads the function pointers. On Retina screens the framebuffer is 2× the window size,
so the viewport always uses `glfwGetFramebufferSize`.

**Camera (`Camera.cpp`)** — orbit: position = target + d·(cos y cos p, sin p, sin y cos p). Fly: forward vector from
yaw/pitch, right = forward × up. View = `lookAt(eye, eye+forward, up)`, projection = `perspective(45°, aspect,
0.05, 100)`. Smoothing: `value += (goal − value)·(1 − e^(−k·dt))` — frame-rate independent because two half-frames
give the same result as one full frame.

**Scene graph (`SceneNode`)**: local = T · R_y · R_x · R_z · S; world = parentWorld · local, computed recursively
every frame. Example chain: desk lamp base → yaw joint → shoulder → elbow → head → shade + bulb. Rotating the
shoulder moves everything after it. The spotlight position is `head.world · (0,0,0,1)` and its direction is
`head.world · (0,−1,0,0)` (w = 0 → direction, not affected by translation).

**Normal matrix**: normals are transformed with (M⁻¹)ᵀ so they stay perpendicular after non-uniform scaling.

**Shadow mapping (`ShadowMap.cpp`, `scene.frag`)**:
1. Render depth from the sun with an orthographic camera (sun light rays are parallel). The ortho box is fitted
   around the 8 corners of the room transformed into light space.
2. In the scene shader: p_light = LightSpace · worldPos → NDC → [0,1]; compare its depth with the stored depth.
3. Acne: slope-scaled `glPolygonOffset` + small bias; normal offset. PCF: 3×3 taps of a `sampler2DShadow`
   (hardware compare + bilinear) → soft edges.
4. The diorama has no ceiling, so invisible "shadow-only" boxes (ceiling, right wall) are drawn only in the
   shadow pass → the sun enters only through the window.

**Q&A to expect**: Why orthographic for the sun? What is shadow acne / peter-panning? Why w = 0 for directions?
What happens if you multiply matrices in the other order?

---

## 2. Zhumagali Yessen — lighting and day cycle

**Blinn-Phong (`scene.frag`)** for each light:
- diffuse = albedo · max(N·L, 0)
- H = normalize(L + V), specular = ks · max(N·H, 0)^shininess
- colour += lightColour · (diffuse + specular) · attenuation · (shadow or cone)

Blinn-Phong uses the half vector H instead of the reflection vector R: cheaper and no highlight cut-off when the
angle between V and R is > 90°.

**Ambient**: hemispheric — mix(groundColour, skyColour, N.y·0.5+0.5): surfaces facing up get sky light,
facing down get floor bounce.

**Lights**: 1 directional (sun by day, moon by night) with shadows, 1 spotlight (desk lamp), up to 23 point lights
(floor lamp, 14 fairy bulbs that twinkle with sin(t), laptop glow).
- Point falloff: (1 − (d/r)⁴)² / (d² + 1) — inverse-square, but reaches exactly 0 at radius r (Karis 2013).
- Spot cone: θ = dot(−L, spotDir); intensity = smoothstep(cos outer, cos inner, θ) — soft edge.
- The spot radius is short (1.2 m) because the lamp has no shadow map; otherwise its light would leak through the
  desk onto the floor (we hit exactly this bug during testing).

**Day cycle (`DayCycle.cpp`)**: t = (hour − 6)/12, elevation = sin(πt)·50°, azimuth from +70° to −70°;
sunDir = (sin az cos el, sin el, −cos az cos el). Colours (zenith, horizon, ambient, grade) are keyframes at
0, 4.8, 6.2, 8, 12, 16.5, 18.2, 19.6, 21, 24 h, interpolated with smoothstep. Sun colour goes from orange (low) to
warm white (high). After sunset the moon (same path, +12 h) becomes the shadow-casting light.

**Linear lighting**: textures are uploaded as sRGB so the GPU converts them to linear; lighting is done in linear
HDR; gamma 1/2.2 only at the very end.

**Q&A**: Phong vs Blinn-Phong? Why do we need normalize after interpolation? Why gamma correction?
What does shininess change?

---

## 3. Abdimutalip Aidana — modelling, textures, sky

**Primitives (`Primitives.cpp`)**: every object is built from box, plane, quad, sphere (latitude/longitude), cylinder
/ truncated cone (with slope-correct normals), torus. Each generator produces position + normal + UV, then
`fixWinding` makes every triangle counter-clockwise relative to its normal.

**Mesh (`Mesh.cpp`)**: one VAO, a VBO with interleaved vertices (location 0 position, 1 normal, 2 UV) and an EBO
with indices. Instanced meshes add a second VBO with a mat4 (locations 3–6) + colour (7) per instance and
`glVertexAttribDivisor(…, 1)`. Books (90+) and fairy bulbs use one draw call each.

**Textures (`Texture.cpp`, `tools/gen_textures.py`)**: loaded with stb_image (flipped vertically because OpenGL's
v = 0 is the bottom), `GL_SRGB8_ALPHA8`, `glGenerateMipmap`, `GL_LINEAR_MIPMAP_LINEAR` (trilinear) + 8× anisotropic,
`GL_REPEAT`. Mipmaps remove shimmering on the floor far away. All textures are generated by our Python script
(value noise, sine stripes, drawing) — no third-party assets.

**UVs without stretching**: boxes get UVs in metres (`uvPerMeter`), so a 2 m desk and a 0.4 m drawer have the same
wood scale. Walls and floor use world-space UVs in the vertex shader (project the world position on the face
plane) so separate wall boxes continue the same wallpaper pattern.

**Sky (`sky.frag`)**: a quad behind the window; colour per pixel from zenith/horizon gradient, sun disc and glow,
moon, hash-based stars, two layers of sine "hills" and village lights at night. Azimuth comes from the camera ray,
so it has parallax like a real view.

**Q&A**: What is a VAO vs VBO vs EBO? What are mipmaps and why trilinear? What does sRGB mean?
How does instancing reduce draw calls?

---

## 4. Zhagypar Aruzhan — particles, post-processing, UI, testing

**Dust (`DustParticles.cpp`, `dust.vert/frag`)**: 6 000 camera-facing quads in ONE instanced draw call. Each
particle has a random seed uploaded once; the vertex shader moves it: p = fract(seed + v·t + small sines) →
wrapped inside the room box. The billboard is built from the camera right/up vectors (rows of the view matrix).
The vertex shader samples the sun's shadow map at the particle position → the dust is bright only inside the beam.
Additive blending, depth test on, depth write off.

**HDR + MSAA**: the scene is rendered into a 4× multisampled RGBA16F framebuffer (values > 1 allowed), then
`glBlitFramebuffer` resolves colour and depth into textures.

**Bloom**: bright pass (soft threshold) at half resolution → separable Gaussian blur (horizontal, vertical,
4 iterations, ping-pong FBOs) → added to the image. Separable = 2·9 samples instead of 81.

**Light shafts (`shafts.frag`)**: for each pixel, reconstruct the world position from depth with the inverse
view-projection matrix, intersect the camera ray with the room box, take 40 steps and count how many are lit
(shadow map). Multiply by the Henyey–Greenstein phase function (g = 0.55: more light when looking towards the sun).
Interleaved gradient noise jitters the start → no banding.

**Tone mapping (`composite.frag`)**: exposure → time-of-day colour grade → ACES filmic curve (maps HDR to 0..1
smoothly) → vignette → gamma 1/2.2.

**UI (`UI.cpp`)**: Dear ImGui panel (time slider, toggles, sliders, camera presets, stats), FPS overlay, help window.
FPS is also in the window title, averaged over 0.5 s.

**Testing**: measure FPS with `--novsync` at 1280×720 on every team laptop and fill the FPS table in the report;
check every toggle; check that a fresh clone builds with the README steps.

**Q&A**: Why HDR? Why blur at half resolution? What is additive blending? Why is the particle buffer never
re-uploaded?
