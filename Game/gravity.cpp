#include "gravity.h"
#include <cmath>
#include <algorithm>

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

void resolve_tile_collision(Object& obj, const TileType* tiles, float obj_w, float obj_h) {
    for (int iter = 0; iter < 3; iter++) { // up to 3 passes for corner cases
        float left   = obj.x;
        float top    = obj.y;
        float right  = obj.x + obj_w;
        float bottom = obj.y + obj_h;

        int tx0 = std::max(0, (int)(left   / BLOCK_SIZE));
        int ty0 = std::max(0, (int)(top    / BLOCK_SIZE));
        int tx1 = std::min((int)GAME_WIDTH  - 1, (int)((right  - 1.0f) / BLOCK_SIZE));
        int ty1 = std::min((int)GAME_HEIGHT - 1, (int)((bottom - 1.0f) / BLOCK_SIZE));

        bool any = false;
        for (int ty = ty0; ty <= ty1; ty++) {
            for (int tx = tx0; tx <= tx1; tx++) {
                if (tiles[ty * GAME_WIDTH + tx] != TileType::SOLID) continue;

                float tile_x = tx * BLOCK_SIZE;
                float tile_y = ty * BLOCK_SIZE;

                float ov_left   = right  - tile_x;
                float ov_right  = tile_x + BLOCK_SIZE - left;
                float ov_top    = bottom - tile_y;
                float ov_bottom = tile_y + BLOCK_SIZE - top;

                float min_x = std::min(ov_left, ov_right);
                float min_y = std::min(ov_top,  ov_bottom);

                if (min_x < min_y) {
                    if (ov_left < ov_right) { obj.x -= ov_left;   obj.vx = std::min(0.0f, obj.vx); }
                    else                    { obj.x += ov_right;  obj.vx = std::max(0.0f, obj.vx); }
                } else {
                    if (ov_top < ov_bottom) { obj.y -= ov_top;    obj.vy = std::min(0.0f, obj.vy); }
                    else                    { obj.y += ov_bottom; obj.vy = std::max(0.0f, obj.vy); }
                }
                any = true;
            }
        }
        if (!any) break;
    }
}
