#pragma once
// All user-controllable settings in one place. Changed by keyboard shortcuts
// (App.cpp) and by the ImGui panel (UI.cpp), read by Scene and Renderer.

struct AppState {
    // time of day
    float hour = 9.0f;            // 0..24
    bool playing = false;         // time-lapse running
    float timeSpeed = 1.0f;       // in-game hours per real second while playing
    bool warmColdGrading = true;  // stretch: warm/cold colour change by time

    // lights (targets; actual brightness fades towards them)
    bool deskLamp = false;
    bool floorLamp = false;
    bool fairyLights = false;
    bool autoLights = true;       // lamps switch on by themselves at dusk

    // animated props
    bool recordPlaying = true;
    float lampYaw = -25.0f;       // desk lamp base rotation (arrow keys)
    float lampHeadPitch = 110.0f; // desk lamp head rotation (arrow keys)

    // rendering features
    bool shadows = true;          // stretch: shadow mapping for the sun
    bool dust = true;             // stretch: dust particles in the light beam
    bool lightShafts = true;      // extra: volumetric light shafts
    bool bloom = true;
    float exposure = 1.0f;
    float dustAmount = 1.0f;
    bool wireframe = false;

    // UI
    bool showPanel = true;
    bool showHelp = false;
    bool screenshotRequested = false;
};
