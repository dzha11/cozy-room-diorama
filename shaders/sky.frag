#version 330 core
// Procedural sky seen through the window: zenith/horizon gradient, sun disc
// with a glow, moon, stars at night and two layers of distant hills.
// Azimuth comes from the camera ray (parallax), elevation from the height on
// the backdrop, so the horizon always sits in the lower part of the window.

in vec3 vWorldPos;
out vec4 FragColor;

uniform vec3 uCameraPos;
uniform vec3 uZenith;
uniform vec3 uHorizon;
uniform float uSunAzimuth;    // radians
uniform float uSunElevation;  // radians
uniform float uSunVisible;
uniform float uMoonAzimuth;
uniform float uMoonElevation;
uniform float uNight;
uniform vec3 uSunColor;
uniform float uTime;

const float PI = 3.14159265;

float hash(vec2 p) { return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453); }

// Angular distance between two (azimuth, elevation) directions.
float angDist(vec2 a, vec2 b) {
    vec3 da = vec3(cos(a.y) * sin(a.x), sin(a.y), cos(a.y) * cos(a.x));
    vec3 db = vec3(cos(b.y) * sin(b.x), sin(b.y), cos(b.y) * cos(b.x));
    return acos(clamp(dot(da, db), -1.0, 1.0));
}

float hills(float az, float base, float amp, float seed) {
    return base + amp * (0.55 * sin(az * 5.0 + seed) + 0.3 * sin(az * 11.0 + seed * 2.3) + 0.15 * sin(az * 23.0 + seed * 4.1));
}

void main() {
    vec3 d = vWorldPos - uCameraPos;
    float az = atan(d.x, -d.z);                    // 0 = straight out of the window
    float el = (vWorldPos.y - 1.25) * 0.55;        // ~ -8 .. +40 degrees across the window
    vec2 dir = vec2(az, el);

    float h = clamp(el / 0.7, 0.0, 1.0);
    vec3 sky = mix(uHorizon, uZenith, pow(h, 0.6));

    // stars
    vec2 cell = floor(dir * vec2(500.0, 180.0));   // azimuth changes slower per pixel
    float star = step(0.994, hash(cell)) * (0.6 + 0.4 * sin(uTime * 3.0 + hash(cell + 7.0) * 30.0));
    sky += vec3(star) * uNight * smoothstep(0.02, 0.15, el) * 1.5;

    // sun: bright disc + wide glow, both tinted with the sun colour
    float ds = angDist(dir, vec2(uSunAzimuth, uSunElevation));
    sky += uSunColor * uSunVisible * (smoothstep(0.050, 0.040, ds) * 18.0 + exp(-ds * 6.0) * 0.6);
    // moon
    float dm = angDist(dir, vec2(uMoonAzimuth, uMoonElevation));
    sky += vec3(0.85, 0.9, 1.0) * uNight * (smoothstep(0.034, 0.028, dm) * 2.5 + exp(-dm * 10.0) * 0.08);

    // far hills (hazy) and near hills (darker), lit a little by the sky
    vec3 farCol = mix(uHorizon, uZenith, 0.35) * 0.55;
    vec3 nearCol = mix(uHorizon, uZenith, 0.5) * 0.22 + vec3(0.02, 0.035, 0.02) * (1.0 - uNight);
    if (el < hills(az, 0.06, 0.045, 1.0)) sky = farCol;
    if (el < hills(az, 0.0, 0.05, 4.0)) {
        sky = nearCol;
        // a few warm village windows at night
        vec2 g = vec2(az * 440.0, el * 160.0);
        vec2 c = floor(g);
        float dotMask = smoothstep(0.35, 0.15, length(fract(g) - 0.5));
        sky += vec3(1.0, 0.65, 0.3) * step(0.975, hash(c + 3.0)) * dotMask * uNight * 1.2 * step(el, -0.01);
    }
    FragColor = vec4(sky, 1.0);
}
