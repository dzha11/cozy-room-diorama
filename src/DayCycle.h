#pragma once
// DayCycle: turns a time of day (0..24 h) into sun/moon direction, light
// colours, ambient colours and sky colours.
// With "warm/cold grading" enabled the colours follow a real day: orange
// sunrise, neutral noon, golden hour, purple dusk and a cold blue night.
// When disabled everything uses neutral white light (to compare in the demo).

#include <glm/glm.hpp>

struct SkyState {
    float hour = 12.0f;
    glm::vec3 sunDir{0, 1, 0};      // unit vector pointing TO the sun
    glm::vec3 moonDir{0, 1, 0};
    float sunElevationDeg = 0.0f;
    float sunAzimuthDeg = 0.0f;
    float sunVisible = 0.0f;        // 0..1, fades at the horizon
    float moonVisible = 0.0f;

    // The directional light that is actually rendered (sun by day, moon by night)
    glm::vec3 lightDir{0, 1, 0};
    glm::vec3 lightColor{1.0f};     // linear, already multiplied by intensity
    bool lightIsSun = true;

    glm::vec3 ambientSky{0.1f};     // hemispheric ambient: from above
    glm::vec3 ambientGround{0.05f}; // from below (floor bounce)
    glm::vec3 zenith{0.2f, 0.4f, 0.9f};
    glm::vec3 horizon{0.7f, 0.8f, 1.0f};
    glm::vec3 grade{1.0f};          // final colour-grading tint
    float night = 0.0f;             // 0 = full day, 1 = full night
};

class DayCycle {
public:
    SkyState evaluate(float hour, bool warmColdGrading) const;
};
