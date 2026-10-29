#include "SceneNode.h"

#include <glm/gtc/matrix_transform.hpp>

SceneNode* SceneNode::add(const std::string& childName, const Mesh* m, const Material& mat,
                          const glm::vec3& pos) {
    auto node = std::make_unique<SceneNode>(childName);
    node->mesh = m;
    node->material = mat;
    node->position = pos;
    node->parent = this;
    children.push_back(std::move(node));
    return children.back().get();
}

glm::mat4 SceneNode::localMatrix() const {
    glm::mat4 m = glm::translate(glm::mat4(1.0f), position);
    m = glm::rotate(m, glm::radians(rotationDeg.y), glm::vec3(0, 1, 0));
    m = glm::rotate(m, glm::radians(rotationDeg.x), glm::vec3(1, 0, 0));
    m = glm::rotate(m, glm::radians(rotationDeg.z), glm::vec3(0, 0, 1));
    return glm::scale(m, scale);
}

void SceneNode::updateWorld(const glm::mat4& parentWorld) {
    world = parentWorld * localMatrix();
    for (auto& c : children) c->updateWorld(world);
}

void SceneNode::forEach(const std::function<void(const SceneNode&)>& fn) const {
    if (!visible) return;  // hiding a node hides its whole subtree
    fn(*this);
    for (const auto& c : children) c->forEach(fn);
}

SceneNode* SceneNode::find(const std::string& n) {
    if (name == n) return this;
    for (auto& c : children)
        if (SceneNode* r = c->find(n)) return r;
    return nullptr;
}
