#include "DayCycle.h"

#include <glm/gtc/constants.hpp>
#include <algorithm>
#include <cmath>

namespace {

struct Key {
    float hour;
    glm::vec3 zenith, horizon, ambient, grade;
};

// sRGB-looking colours chosen by eye; converted to linear below.
const Key kKeys[] = {
    {0.0f,  {0.010f, 0.015f, 0.050f}, {0.040f, 0.050f, 0.120f}, {0.10f, 0.13f, 0.28f}, {0.80f, 0.90f, 1.20f}},
    {4.8f,  {0.020f, 0.025f, 0.080f}, {0.090f, 0.080f, 0.180f}, {0.12f, 0.13f, 0.28f}, {0.85f, 0.90f, 1.15f}},
    {6.2f,  {0.180f, 0.220f, 0.450f}, {1.000f, 0.520f, 0.320f}, {0.55f, 0.42f, 0.40f}, {1.15f, 0.98f, 0.85f}},
    {8.0f,  {0.280f, 0.480f, 0.850f}, {0.900f, 0.800f, 0.700f}, {0.60f, 0.62f, 0.66f}, {1.06f, 1.00f, 0.94f}},
    {12.0f, {0.220f, 0.480f, 0.950f}, {0.680f, 0.820f, 1.000f}, {0.66f, 0.72f, 0.82f}, {1.00f, 1.00f, 1.02f}},
    {16.5f, {0.250f, 0.450f, 0.850f}, {0.950f, 0.800f, 0.600f}, {0.64f, 0.62f, 0.60f}, {1.06f, 1.00f, 0.93f}},
    {18.2f, {0.300f, 0.250f, 0.500f}, {1.000f, 0.420f, 0.180f}, {0.56f, 0.44f, 0.42f}, {1.14f, 0.97f, 0.84f}},
    {19.6f, {0.060f, 0.050f, 0.160f}, {0.300f, 0.140f, 0.220f}, {0.22f, 0.17f, 0.30f}, {1.00f, 0.90f, 1.05f}},
    {21.0f, {0.012f, 0.018f, 0.060f}, {0.050f, 0.060f, 0.140f}, {0.11f, 0.13f, 0.28f}, {0.82f, 0.90f, 1.20f}},
    {24.0f, {0.010f, 0.015f, 0.050f}, {0.040f, 0.050f, 0.120f}, {0.10f, 0.13f, 0.28f}, {0.80f, 0.90f, 1.20f}},
};

glm::vec3 toLinear(const glm::vec3& c) { return glm::pow(c, glm::vec3(2.2f)); }

// Sun (or moon) direction for a given hour. The sun rises at 6:00 on the +x
// side (east), passes over the back of the room (-z, where the window is) and
// sets at 18:00 on the -x side. Max elevation ~50 deg so it shines in through
// the window and the beam travels across the floor during the day.
glm::vec3 celestialDir(float hour, float& elevationDeg, float& azimuthDeg) {
    const float PI = glm::pi<float>();
    float t = (hour - 6.0f) / 12.0f;  // 0 at sunrise, 1 at sunset
    elevationDeg = std::sin(PI * t) * 50.0f;
    azimuthDeg = 70.0f - 140.0f * std::clamp(t, -0.25f, 1.25f);
    float el = glm::radians(elevationDeg), az = glm::radians(azimuthDeg);
    return glm::normalize(glm::vec3(std::sin(az) * std::cos(el), std::sin(el), -std::cos(az) * std::cos(el)));
}

}  // namespace

SkyState DayCycle::evaluate(float hour, bool grading) const {
    SkyState s;
    hour = std::fmod(std::fmod(hour, 24.0f) + 24.0f, 24.0f);
    s.hour = hour;

    s.sunDir = celestialDir(hour, s.sunElevationDeg, s.sunAzimuthDeg);
    float moonEl = 0.0f, moonAz = 0.0f;
    s.moonDir = celestialDir(std::fmod(hour + 12.0f, 24.0f), moonEl, moonAz);

    s.sunVisible = glm::smoothstep(-4.0f, 6.0f, s.sunElevationDeg);
    s.moonVisible = glm::smoothstep(-4.0f, 8.0f, moonEl);
    s.night = 1.0f - glm::smoothstep(-8.0f, 4.0f, s.sunElevationDeg);

    // interpolate the colour keys
    int i = 0;
    while (i + 1 < int(sizeof(kKeys) / sizeof(kKeys[0])) - 1 && hour >= kKeys[i + 1].hour) ++i;
    const Key& a = kKeys[i];
    const Key& b = kKeys[i + 1];
    float f = glm::smoothstep(0.0f, 1.0f, (hour - a.hour) / (b.hour - a.hour));
    s.zenith = toLinear(glm::mix(a.zenith, b.zenith, f));
    s.horizon = toLinear(glm::mix(a.horizon, b.horizon, f));
    glm::vec3 ambient = toLinear(glm::mix(a.ambient, b.ambient, f));
    s.grade = glm::mix(a.grade, b.grade, f);

    // sun colour: deep orange near the horizon, warm white high in the sky
    float high = glm::smoothstep(2.0f, 35.0f, s.sunElevationDeg);
    glm::vec3 sunCol = toLinear(glm::mix(glm::vec3(1.0f, 0.50f, 0.22f), glm::vec3(1.0f, 0.95f, 0.86f), high));
    glm::vec3 moonCol = toLinear(glm::vec3(0.55f, 0.65f, 1.0f));

    if (!grading) {  // neutral comparison mode
        sunCol = glm::vec3(1.0f);
        moonCol = glm::vec3(0.8f);
        float lum = glm::dot(ambient, glm::vec3(0.2126f, 0.7152f, 0.0722f));
        ambient = glm::vec3(lum);
        s.grade = glm::vec3(1.0f);
    }

    if (s.sunElevationDeg > -1.0f) {
        s.lightIsSun = true;
        s.lightDir = s.sunDir;
        s.lightColor = sunCol * (4.2f * s.sunVisible);
    } else {
        s.lightIsSun = false;
        s.lightDir = s.moonDir;
        s.lightColor = moonCol * (0.55f * s.moonVisible);
    }
    if (s.lightDir.y < 0.06f) {  // keep the shadow camera above the horizon
        s.lightDir.y = 0.06f;
        s.lightDir = glm::normalize(s.lightDir);
    }

    float ambientStrength = glm::mix(0.55f, 0.26f, s.night);
    s.ambientSky = ambient * ambientStrength;
    s.ambientGround = ambient * ambientStrength * 0.45f;
    return s;
}
