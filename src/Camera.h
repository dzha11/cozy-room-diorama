#pragma once
// Camera with two modes:
//   Orbit - rotates around a target point (default diorama view)
//   Fly   - free FPS-style camera (WASD + mouse look)
// All motion is scaled by delta time and smoothed with an exponential filter,
// so it behaves the same at 30 or 300 FPS.

#include <glm/glm.hpp>

struct CameraInput {
    glm::vec2 mouseDelta{0.0f};  // pixels since last frame (only when rotating)
    float scroll = 0.0f;
    bool forward = false, back = false, left = false, right = false, up = false, down = false;
    bool fast = false;
};

class Camera {
public:
    enum class Mode { Orbit, Fly };

    Camera();

    void update(float dt, const CameraInput& in);
    void setMode(Mode m);
    Mode mode() const { return m_mode; }
    void toggleMode() { setMode(m_mode == Mode::Orbit ? Mode::Fly : Mode::Orbit); }

    // Smoothly moves the orbit camera to a preset (target, yaw, pitch, distance).
    void goToPreset(int index);
    void snapToGoal();   // jump to the goal immediately (no smoothing)

    glm::mat4 view() const;
    glm::mat4 projection(float aspect) const;
    glm::vec3 position() const { return m_position; }
    glm::vec3 forward() const;
    float fovDeg() const { return m_fov; }
    float nearPlane() const { return m_near; }
    float farPlane() const { return m_far; }

private:
    void updateOrbitPosition();

    Mode m_mode = Mode::Orbit;
    // orbit parameters (current and goal values, current follows goal)
    glm::vec3 m_target, m_goalTarget;
    float m_yaw, m_pitch, m_distance;
    float m_goalYaw, m_goalPitch, m_goalDistance;
    // fly parameters
    glm::vec3 m_position{0.0f};
    glm::vec3 m_velocity{0.0f};
    float m_flyYaw = -90.0f, m_flyPitch = 0.0f;

    float m_fov = 45.0f, m_near = 0.05f, m_far = 100.0f;
};
