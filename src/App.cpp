#include "App.h"
#include "Renderer.h"
#include "Scene.h"
#include "UI.h"

#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <stb_image_write.h>

#include <algorithm>
#include <cmath>
#include <ctime>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace {
App* appFrom(GLFWwindow* w) { return static_cast<App*>(glfwGetWindowUserPointer(w)); }
}  // namespace

App::App(const LaunchOptions& opt) : m_opt(opt) {
    glfwSetErrorCallback([](int code, const char* msg) { std::cerr << "GLFW error " << code << ": " << msg << "\n"; });
    if (!glfwInit()) throw std::runtime_error("Failed to initialise GLFW");

    // OpenGL 3.3 core profile; forward-compatible is required on macOS
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
    m_window = glfwCreateWindow(opt.width, opt.height, "Cozy Room Diorama", nullptr, nullptr);
    if (!m_window) throw std::runtime_error("Failed to create a window with OpenGL 3.3 core");
    glfwMakeContextCurrent(m_window);
    glfwSwapInterval(opt.noVsync ? 0 : 1);
    if (!gladLoadGL(glfwGetProcAddress)) throw std::runtime_error("Failed to load OpenGL functions (GLAD)");
    std::cout << "OpenGL " << glGetString(GL_VERSION) << " on " << glGetString(GL_RENDERER) << "\n";

    glfwSetWindowUserPointer(m_window, this);
    glfwSetKeyCallback(m_window, [](GLFWwindow* w, int key, int, int action, int mods) {
        appFrom(w)->onKey(key, action, mods);
    });
    glfwSetScrollCallback(m_window, [](GLFWwindow* w, double, double dy) { appFrom(w)->onScroll(dy); });

    m_scene = std::make_unique<Scene>();
    m_renderer = std::make_unique<Renderer>();
    // ImGui installs its own callbacks and chains to the ones set above
    m_ui = std::make_unique<UI>(m_window);

    if (opt.hour >= 0.0f) m_state.hour = opt.hour;
    m_camera.goToPreset(opt.preset);
    m_camera.snapToGoal();
    if (opt.lightsOn) {
        m_state.deskLamp = m_state.floorLamp = m_state.fairyLights = true;
        m_state.autoLights = false;
    }
    if (opt.noShadows) m_state.shadows = false;
    if (opt.noDust) m_state.dust = false;
    if (opt.noShafts) m_state.lightShafts = false;
    if (opt.noGrading) m_state.warmColdGrading = false;
    if (opt.wireframe) m_state.wireframe = true;
    if (!opt.capturePath.empty()) m_state.showPanel = opt.withUI;
    updateAutoLights();
}

App::~App() {
    m_ui.reset();
    m_renderer.reset();
    m_scene.reset();
    if (m_window) glfwDestroyWindow(m_window);
    glfwTerminate();
}

void App::onScroll(double dy) {
    if (!m_ui->wantsMouse()) m_scroll += static_cast<float>(dy);
}

void App::onKey(int key, int action, int mods) {
    if (action != GLFW_PRESS) return;
    if (m_ui->wantsKeyboard()) return;
    AppState& s = m_state;
    switch (key) {
        case GLFW_KEY_ESCAPE:
            if (m_camera.mode() == Camera::Mode::Fly) {
                m_camera.setMode(Camera::Mode::Orbit);
                glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            } else {
                glfwSetWindowShouldClose(m_window, GLFW_TRUE);
            }
            break;
        case GLFW_KEY_C:
            m_camera.toggleMode();
            glfwSetInputMode(m_window, GLFW_CURSOR,
                             m_camera.mode() == Camera::Mode::Fly ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
            glfwGetCursorPos(m_window, &m_lastX, &m_lastY);
            break;
        case GLFW_KEY_1: case GLFW_KEY_2: case GLFW_KEY_3: case GLFW_KEY_4:
            m_camera.goToPreset(key - GLFW_KEY_1);
            glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            break;
        case GLFW_KEY_P: s.playing = !s.playing; break;
        case GLFW_KEY_L: s.deskLamp = !s.deskLamp; break;
        case GLFW_KEY_K: s.floorLamp = !s.floorLamp; break;
        case GLFW_KEY_F: s.fairyLights = !s.fairyLights; break;
        case GLFW_KEY_N: s.autoLights = !s.autoLights; m_autoPhase = -1; break;
        case GLFW_KEY_M: s.recordPlaying = !s.recordPlaying; break;
        case GLFW_KEY_G: s.shadows = !s.shadows; break;
        case GLFW_KEY_B: s.dust = !s.dust; break;
        case GLFW_KEY_V: s.lightShafts = !s.lightShafts; break;
        case GLFW_KEY_T: s.warmColdGrading = !s.warmColdGrading; break;
        case GLFW_KEY_X: s.wireframe = !s.wireframe; break;
        case GLFW_KEY_H: s.showHelp = !s.showHelp; break;
        case GLFW_KEY_TAB: s.showPanel = !s.showPanel; break;
        case GLFW_KEY_F12: s.screenshotRequested = true; break;
        default: break;
    }
    (void)mods;
}

void App::processContinuousInput(float dt, CameraInput& in) {
    double x, y;
    glfwGetCursorPos(m_window, &x, &y);
    glm::vec2 delta(float(x - m_lastX), float(y - m_lastY));
    m_lastX = x;
    m_lastY = y;

    if (m_camera.mode() == Camera::Mode::Fly) {
        in.mouseDelta = delta;
    } else {
        bool down = glfwGetMouseButton(m_window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS ||
                    glfwGetMouseButton(m_window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;
        if (down && !m_dragging && !m_ui->wantsMouse()) m_dragging = true;
        if (!down) m_dragging = false;
        if (m_dragging) in.mouseDelta = delta;
    }
    in.scroll = m_scroll;
    m_scroll = 0.0f;

    if (m_ui->wantsKeyboard()) return;
    auto key = [&](int k) { return glfwGetKey(m_window, k) == GLFW_PRESS; };
    in.forward = key(GLFW_KEY_W);
    in.back = key(GLFW_KEY_S);
    in.left = key(GLFW_KEY_A);
    in.right = key(GLFW_KEY_D);
    in.up = key(GLFW_KEY_E);
    in.down = key(GLFW_KEY_Q);
    in.fast = key(GLFW_KEY_LEFT_SHIFT) || key(GLFW_KEY_RIGHT_SHIFT);

    // time of day: hold [ or ] (3 hours per second)
    if (key(GLFW_KEY_RIGHT_BRACKET)) m_state.hour += 3.0f * dt;
    if (key(GLFW_KEY_LEFT_BRACKET)) m_state.hour -= 3.0f * dt;
    // aim the desk lamp (rotates two joints of the hierarchy)
    if (key(GLFW_KEY_LEFT)) m_state.lampYaw += 60.0f * dt;
    if (key(GLFW_KEY_RIGHT)) m_state.lampYaw -= 60.0f * dt;
    if (key(GLFW_KEY_UP)) m_state.lampHeadPitch -= 50.0f * dt;
    if (key(GLFW_KEY_DOWN)) m_state.lampHeadPitch += 50.0f * dt;
    m_state.lampYaw = std::clamp(m_state.lampYaw, -80.0f, 80.0f);
    m_state.lampHeadPitch = std::clamp(m_state.lampHeadPitch, 60.0f, 170.0f);
}

// Lamps switch on one after another at dusk and off at dawn. Only the
// transitions change the switches, so the user can still override them.
void App::updateAutoLights() {
    if (!m_state.autoLights) return;
    const float h = m_state.hour;
    // phase 0 = day, 1 = fairy lights, 2 = + floor lamp, 3 = + desk lamp
    int phase = 0;
    if (h >= 18.3f || h < 6.6f) phase = 1;
    if (h >= 18.9f || h < 6.3f) phase = 2;
    if (h >= 19.5f || h < 6.0f) phase = 3;
    if (h >= 23.6f || h < 5.5f) phase = 2;   // desk lamp off late at night
    if (phase == m_autoPhase) return;
    m_autoPhase = phase;
    m_state.fairyLights = phase >= 1;
    m_state.floorLamp = phase >= 2;
    m_state.deskLamp = phase == 3;
}

void App::updateTitle(float dt) {
    m_fpsTimer += dt;
    ++m_fpsFrames;
    if (m_fpsTimer >= 0.5f) {
        m_fps = m_fpsFrames / m_fpsTimer;
        m_frameMs = 1000.0f * m_fpsTimer / m_fpsFrames;
        m_fpsTimer = 0.0f;
        m_fpsFrames = 0;
        char title[128];
        std::snprintf(title, sizeof(title), "Cozy Room Diorama  |  %.1f FPS  |  %.2f ms", m_fps, m_frameMs);
        glfwSetWindowTitle(m_window, title);
    }
}

void App::saveScreenshot(const std::string& requested) {
    int w, h;
    glfwGetFramebufferSize(m_window, &w, &h);
    std::vector<unsigned char> px(size_t(w) * h * 3);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadBuffer(GL_BACK);
    glReadPixels(0, 0, w, h, GL_RGB, GL_UNSIGNED_BYTE, px.data());
    std::string path = requested;
    if (path.empty()) {
        std::filesystem::create_directories("screenshots");
        char name[64];
        std::time_t t = std::time(nullptr);
        std::strftime(name, sizeof(name), "screenshots/shot_%Y%m%d_%H%M%S.png", std::localtime(&t));
        path = name;
    }
    stbi_flip_vertically_on_write(1);
    if (stbi_write_png(path.c_str(), w, h, 3, px.data(), w * 3))
        std::cout << "Saved screenshot " << path << " (" << w << "x" << h << ")\n";
    else
        std::cerr << "Could not write " << path << "\n";
}

int App::run() {
    double last = glfwGetTime();
    float time = 0.0f;
    int frame = 0;
    const bool capturing = !m_opt.capturePath.empty();

    while (!glfwWindowShouldClose(m_window)) {
        glfwPollEvents();
        double now = glfwGetTime();
        float realDt = float(now - last);
        float dt = std::min(realDt, 0.1f);
        last = now;
        if (capturing) dt = 1.0f / 60.0f;   // deterministic captures
        time += dt;

        CameraInput camIn;
        processContinuousInput(dt, camIn);
        if (m_state.playing) m_state.hour += m_state.timeSpeed * dt;
        m_state.hour = std::fmod(std::fmod(m_state.hour, 24.0f) + 24.0f, 24.0f);
        updateAutoLights();

        m_camera.update(dt, camIn);
        SkyState sky = m_dayCycle.evaluate(m_state.hour, m_state.warmColdGrading);
        m_scene->update(dt, time, m_state, sky);
        LightSet lights = m_scene->lights(sky);

        int fbW, fbH;
        glfwGetFramebufferSize(m_window, &fbW, &fbH);   // pixels (Retina = 2x window size)
        m_renderer->render(*m_scene, m_camera, m_state, sky, lights, fbW, fbH, time);

        // screenshots are taken before the UI so figures are clean
        bool lastCaptureFrame = capturing && frame == m_opt.captureFrames;
        if (m_state.screenshotRequested && !capturing) {
            saveScreenshot("");
            m_state.screenshotRequested = false;
        }

        UIStats st;
        st.fps = m_fps;
        st.frameMs = m_frameMs;
        st.fbWidth = fbW;
        st.fbHeight = fbH;
        st.drawCalls = m_renderer->stats().drawCalls;
        st.pointLights = m_renderer->stats().pointLights;
        st.objects = m_scene->objectCount();
        st.particles = m_state.dust ? m_renderer->dustCount() : 0;
        if (!capturing || m_opt.withUI) m_ui->draw(m_state, m_camera, st);
        if (lastCaptureFrame) {
            saveScreenshot(m_opt.capturePath);
            break;
        }

        glfwSwapBuffers(m_window);
        updateTitle(realDt);
        ++frame;
    }
    return 0;
}
