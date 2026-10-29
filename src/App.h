#pragma once
// App: window + OpenGL context (GLFW/GLAD), input handling, the main loop,
// FPS counter, time-of-day logic and screenshots.

#include "AppState.h"
#include "Camera.h"
#include "DayCycle.h"

#include <memory>
#include <string>

struct GLFWwindow;
class Scene;
class Renderer;
class UI;

struct LaunchOptions {
    // optional automatic capture (used to make report figures / test builds):
    //   CozyRoom --capture out.png --hour 19.5 --preset 0 --frames 60 --lights
    std::string capturePath;
    int captureFrames = 60;
    int preset = 0;
    float hour = -1.0f;
    bool lightsOn = false;
    bool withUI = false;
    bool noVsync = false;
    int width = 1280, height = 720;
};

class App {
public:
    explicit App(const LaunchOptions& opt);
    ~App();
    int run();

    // called from GLFW callbacks
    void onKey(int key, int action, int mods);
    void onScroll(double dy);

private:
    void processContinuousInput(float dt, CameraInput& camIn);
    void updateAutoLights();
    void saveScreenshot(const std::string& path);
    void updateTitle(float dt);

    LaunchOptions m_opt;
    GLFWwindow* m_window = nullptr;
    AppState m_state;
    Camera m_camera;
    DayCycle m_dayCycle;
    std::unique_ptr<Scene> m_scene;
    std::unique_ptr<Renderer> m_renderer;
    std::unique_ptr<UI> m_ui;

    double m_lastX = 0.0, m_lastY = 0.0;
    bool m_dragging = false;
    float m_scroll = 0.0f;
    int m_autoPhase = -1;    // last automatic lighting phase (to only act on changes)

    float m_fpsTimer = 0.0f;
    int m_fpsFrames = 0;
    float m_fps = 0.0f, m_frameMs = 0.0f;
};
