#pragma once

#include <glm/glm.hpp>

class SimplePhysicalObject
{
private:
public:
    glm::vec2 position{ 0.0f, 0.0f }, last_position{ 0.0f, 0.0f }, acceleration{ 0.0f, 0.0f };
    glm::vec3 color{ 0.0f, 0.0f, 0.0f };

    static constexpr glm::vec2 size = { 1.0f, 1.0f };
    static constexpr float movementDamping = 40.0f; // �˶�ʱ�������С���������ٶȳ������Ҵ������ٶȷ���ķ����������deltaTime�̶��ĳ��ϣ��������䵥λΪ/s^2���Ӷ�����������ȷ���������ٶ�

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