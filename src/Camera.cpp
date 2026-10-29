#include "Camera.h"

#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cmath>

namespace {
// Frame-rate independent smoothing factor: fraction of the remaining distance
// covered this frame. Same result regardless of how dt is split up.
float smoothing(float sharpness, float dt) { return 1.0f - std::exp(-sharpness * dt); }

glm::vec3 dirFromYawPitch(float yawDeg, float pitchDeg) {
    float y = glm::radians(yawDeg), p = glm::radians(pitchDeg);
    return glm::normalize(glm::vec3(std::cos(y) * std::cos(p), std::sin(p), std::sin(y) * std::cos(p)));
}

struct Preset { glm::vec3 target; float yaw, pitch, distance; };
const Preset kPresets[] = {
    {{0.0f, 1.1f, -0.6f}, 62.0f, 26.0f, 11.5f},   // 1: whole diorama
    {{0.4f, 1.2f, -3.0f}, 85.0f, 12.0f, 4.6f},    // 2: window and desk
    {{-2.6f, 1.2f, -2.4f}, 40.0f, 18.0f, 5.2f},   // 3: bed and fairy lights
    {{2.8f, 1.2f, -3.0f}, 105.0f, 14.0f, 4.4f},   // 4: record player and clock
};
}  // namespace

Camera::Camera() {
    const Preset& p = kPresets[0];
    m_target = m_goalTarget = p.target;
    m_yaw = m_goalYaw = p.yaw;
    m_pitch = m_goalPitch = p.pitch;
    m_distance = m_goalDistance = p.distance;
    updateOrbitPosition();
}

void Camera::goToPreset(int index) {
    const int count = static_cast<int>(sizeof(kPresets) / sizeof(kPresets[0]));
    if (index < 0 || index >= count) return;
    setMode(Mode::Orbit);
    const Preset& p = kPresets[index];
    m_goalTarget = p.target;
    m_goalYaw = p.yaw;
    m_goalPitch = p.pitch;
    m_goalDistance = p.distance;
}

void Camera::snapToGoal() {
    m_target = m_goalTarget;
    m_yaw = m_goalYaw;
    m_pitch = m_goalPitch;
    m_distance = m_goalDistance;
    updateOrbitPosition();
}

void Camera::setMode(Mode m) {
    if (m == m_mode) return;
    if (m == Mode::Fly) {
        // continue looking in the same direction as the orbit camera
        glm::vec3 f = glm::normalize(m_target - m_position);
        m_flyYaw = glm::degrees(std::atan2(f.z, f.x));
        m_flyPitch = glm::degrees(std::asin(f.y));
        m_velocity = glm::vec3(0.0f);
    } else {
        // orbit around a point in front of the fly camera
        m_goalTarget = m_target = m_position + forward() * m_distance;
        glm::vec3 back = glm::normalize(m_position - m_target);
        m_goalYaw = m_yaw = glm::degrees(std::atan2(back.z, back.x));
        m_goalPitch = m_pitch = glm::degrees(std::asin(back.y));
    }
    m_mode = m;
}

void Camera::update(float dt, const CameraInput& in) {
    const float speedMul = in.fast ? 2.5f : 1.0f;
    if (m_mode == Mode::Orbit) {
        const float sens = 0.25f;  // degrees per pixel
        m_goalYaw += in.mouseDelta.x * sens;
        m_goalPitch += in.mouseDelta.y * sens;
        // keyboard orbit: A/D yaw, W/S pitch, Q/E zoom (degrees or metres per second)
        m_goalYaw += ((in.right ? 1.0f : 0.0f) - (in.left ? 1.0f : 0.0f)) * 70.0f * speedMul * dt;
        m_goalPitch += ((in.forward ? 1.0f : 0.0f) - (in.back ? 1.0f : 0.0f)) * 45.0f * speedMul * dt;
        m_goalDistance *= std::pow(0.88f, in.scroll);
        m_goalDistance *= std::pow(1.6f, ((in.down ? 1.0f : 0.0f) - (in.up ? 1.0f : 0.0f)) * speedMul * dt);
        m_goalPitch = std::clamp(m_goalPitch, -5.0f, 85.0f);
        m_goalDistance = std::clamp(m_goalDistance, 1.5f, 25.0f);

        float k = smoothing(10.0f, dt);
        m_yaw += (m_goalYaw - m_yaw) * k;
        m_pitch += (m_goalPitch - m_pitch) * k;
        m_distance += (m_goalDistance - m_distance) * k;
        m_target += (m_goalTarget - m_target) * smoothing(5.0f, dt);
        updateOrbitPosition();
    } else {
        const float sens = 0.12f;
        m_flyYaw += in.mouseDelta.x * sens;
        m_flyPitch = std::clamp(m_flyPitch - in.mouseDelta.y * sens, -89.0f, 89.0f);

        glm::vec3 f = forward();
        glm::vec3 r = glm::normalize(glm::cross(f, glm::vec3(0, 1, 0)));
        glm::vec3 wish(0.0f);
        if (in.forward) wish += f;
        if (in.back) wish -= f;
        if (in.right) wish += r;
        if (in.left) wish -= r;
        if (in.up) wish.y += 1.0f;
        if (in.down) wish.y -= 1.0f;
        if (glm::length(wish) > 0.0f) wish = glm::normalize(wish);
        glm::vec3 goalVel = wish * 2.5f * speedMul;  // metres per second
        m_velocity += (goalVel - m_velocity) * smoothing(12.0f, dt);
        m_position += m_velocity * dt;
    }
}

void Camera::updateOrbitPosition() {
    // yaw/pitch describe where the camera sits as seen from the target
    m_position = m_target + dirFromYawPitch(m_yaw, m_pitch) * m_distance;
}

glm::vec3 Camera::forward() const {
    if (m_mode == Mode::Orbit) return glm::normalize(m_target - m_position);
    return dirFromYawPitch(m_flyYaw, m_flyPitch);
}

glm::mat4 Camera::view() const {
    return glm::lookAt(m_position, m_position + forward(), glm::vec3(0, 1, 0));
}

glm::mat4 Camera::projection(float aspect) const {
    return glm::perspective(glm::radians(m_fov), aspect, m_near, m_far);
}
