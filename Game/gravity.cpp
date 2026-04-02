#include "gravity.h"
#include <cmath>

std::vector<Vec2> compute_fk(const Arm& arm) {
    std::vector<Vec2> joints;
    joints.reserve(arm.segments.size() + 1);
    joints.push_back({arm.base_x, arm.base_y});

    float cumulative = arm.base_angle;
    for (const auto& seg : arm.segments) {
        cumulative += seg.angle;
        const Vec2& prev = joints.back();
        joints.push_back({
            prev.x + std::cos(cumulative) * seg.length,
            prev.y + std::sin(cumulative) * seg.length
        });
    }
    return joints;
}

Vec2 arm_tip(const Arm& arm) {
    return compute_fk(arm).back();
}

void update_object(Object& obj, float dt) {
    if (obj.grabbed) return;
    obj.vy += GRAVITY * dt;
    obj.x  += obj.vx * dt;
    obj.y  += obj.vy * dt;

    // Clamp to world bounds
    if (obj.x < 0.0f)                                { obj.x = 0.0f;                                   obj.vx = 0.0f; }
    if (obj.y < 0.0f)                                { obj.y = 0.0f;                                   obj.vy = 0.0f; }
    if (obj.x + OBJ_W > GAME_WIDTH  * BLOCK_SIZE)   { obj.x = GAME_WIDTH  * BLOCK_SIZE - OBJ_W;  obj.vx = 0.0f; }
    if (obj.y + OBJ_H > GAME_HEIGHT * BLOCK_SIZE)   { obj.y = GAME_HEIGHT * BLOCK_SIZE - OBJ_H;  obj.vy = 0.0f; }
}
