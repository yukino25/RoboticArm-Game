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
}
