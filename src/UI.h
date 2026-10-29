#pragma once
// Dear ImGui control panel, FPS overlay and the help window.

#include "AppState.h"
#include "Camera.h"

struct GLFWwindow;

struct UIStats {
    float fps = 0.0f;
    float frameMs = 0.0f;
    int fbWidth = 0, fbHeight = 0;
    int drawCalls = 0;
    int pointLights = 0;
    int objects = 0;
    int particles = 0;
};

class UI {
public:
    explicit UI(GLFWwindow* window);
    ~UI();
    void draw(AppState& st, Camera& cam, const UIStats& stats);
    bool wantsMouse() const;
    bool wantsKeyboard() const;
};
