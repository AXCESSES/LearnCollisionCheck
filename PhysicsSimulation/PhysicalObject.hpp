#pragma once

#include <glm/glm.hpp>

class SimplePhysicalObject
{
private:
public:
    glm::vec2 position{ 0.0f, 0.0f }, last_position{ 0.0f, 0.0f }, acceleration{ 0.0f, 0.0f };
    glm::vec3 color{ 0.0f, 0.0f, 0.0f };

    static constexpr glm::vec2 size = { 1.0f, 1.0f };
    static constexpr float movementDamping = 40.0f; // 运动时的阻尼大小，因其与速度成正比且存在于速度方向的反方向，因此在deltaTime固定的场合，可设置其单位为/s^2，从而在物理上正确地削弱加速度

    SimplePhysicalObject() = default;

    explicit SimplePhysicalObject(glm::vec2 position_) : position(position_), last_position(position_) {
    }

    virtual ~SimplePhysicalObject() {
    }

    void Update(float deltaTime) {
        const glm::vec2 last_update_move = position - last_position;
        const glm::vec2 new_position = position + last_update_move + (acceleration - last_update_move * movementDamping) * (deltaTime * deltaTime); // p2 = p1 + v * t + 1 / 2 * a * t ^ 2
        last_position = position;
        position = new_position;
        acceleration = { 0.0f, 0.0f };
    }
};