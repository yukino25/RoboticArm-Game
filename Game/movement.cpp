// Game/movement.cpp
#include "movement.h"
#include "gravity.h"
#include <algorithm>
#include <cmath>

void cycle_selection(GameState& gs, int direction) {
    Level& level = gs.level;
    int num_arms = (int)level.arms.size();
    Arm& arm = level.arms[level.active_arm];
    int n = (int)arm.segments.size();
    bool has_base = arm.track.has_value();

    int min_idx = has_base ? -1 : 0;
    int new_seg = arm.active_segment + direction;

    if (new_seg < min_idx) {
        // Move to previous arm
        if (num_arms > 1) {
            level.active_arm = (level.active_arm - 1 + num_arms) % num_arms;
            Arm& prev_arm = level.arms[level.active_arm];
            prev_arm.active_segment = (int)prev_arm.segments.size() - 1;
        } else {
            arm.active_segment = n - 1; // wrap within single arm
        }
    } else if (new_seg >= n) {
        // Move to next arm
        if (num_arms > 1) {
            level.active_arm = (level.active_arm + 1) % num_arms;
            Arm& next_arm = level.arms[level.active_arm];
            next_arm.active_segment = next_arm.track.has_value() ? -1 : 0;
        } else {
            arm.active_segment = min_idx; // wrap within single arm
        }
    } else {
        arm.active_segment = new_seg;
    }
}

// Returns true if any joint of `arm` overlaps a solid tile.
static bool joints_hit_tiles(const Arm& arm, const TileType* tiles) {
    auto joints = compute_fk(arm);
    for (const auto& j : joints) {
        int tx = (int)(j.x / BLOCK_SIZE);
        int ty = (int)(j.y / BLOCK_SIZE);
        if (tx < 0 || ty < 0 || tx >= (int)GAME_WIDTH || ty >= (int)GAME_HEIGHT)
            continue; // out-of-bounds joints are allowed (arm can poke outside world edge)
        if (tiles[ty * GAME_WIDTH + tx] == TileType::SOLID)
            return true;
    }
    return false;
}

float clamp_segment_delta(
    Arm arm,
    int seg_idx,
    bool is_angle,
    float requested_delta,
    const TileType* tiles,
    const std::vector<Arm>& all_arms,
    int moving_arm_idx)
{
    if (requested_delta == 0.0f) return 0.0f;

    float sign         = (requested_delta > 0.0f) ? 1.0f : -1.0f;
    float abs_request  = std::abs(requested_delta);

    // Pre-compute foreign arms' joints (they don't move)
    std::vector<std::vector<Vec2>> foreign_joints;
    for (int ai = 0; ai < (int)all_arms.size(); ai++) {
        if (ai == moving_arm_idx) continue;
        foreign_joints.push_back(compute_fk(all_arms[ai]));
    }

    // Helper: apply unsigned delta to a copy of arm
    auto apply_delta = [&](Arm a, float d) -> Arm {
        if (seg_idx >= 0 && seg_idx < (int)a.segments.size()) {
            if (is_angle) a.segments[seg_idx].angle  += sign * d;
            else          a.segments[seg_idx].length  = std::max(MIN_SEG_LEN, a.segments[seg_idx].length + sign * d);
        }
        return a;
    };

    // Pass 1: tile collision — binary search over [0, abs_request]
    float lo = 0.0f, hi = abs_request;
    for (int i = 0; i < SEGMENT_COLLISION_ITERATIONS; i++) {
        float mid = (lo + hi) / 2.0f;
        if (joints_hit_tiles(apply_delta(arm, mid), tiles)) hi = mid;
        else                                                 lo = mid;
    }

    // Pass 2: arm-to-arm — binary search over [0, lo] (tile-safe upper bound)
    float arm_lo = 0.0f, arm_hi = lo;
    for (int i = 0; i < SEGMENT_COLLISION_ITERATIONS; i++) {
        float mid = (arm_lo + arm_hi) / 2.0f;
        auto joints_a = compute_fk(apply_delta(arm, mid));
        bool collides = false;
        for (const auto& jb_list : foreign_joints) {
            for (const auto& ja : joints_a) {
                for (const auto& jb : jb_list) {
                    float dx = ja.x - jb.x, dy = ja.y - jb.y;
                    if (dx*dx + dy*dy < ARM_RADIUS * ARM_RADIUS) {
                        collides = true;
                        break;
                    }
                }
                if (collides) break;
            }
            if (collides) break;
        }
        if (collides) arm_hi = mid;
        else          arm_lo = mid;
    }

    return sign * arm_lo;
}

void apply_movement(
    Arm& arm,
    float delta_angle,
    float delta_extend,
    float delta_track,
    const TileType* tiles,
    const std::vector<Arm>& all_arms,
    int moving_arm_idx)
{
    if (arm.active_segment == -1) {
        if (!arm.track.has_value()) return;
        const Track& t = arm.track.value();
        // Track base movement: tile-clamp only (no arm-to-arm on base)
        if (t.horizontal) {
            float new_x = std::clamp(arm.base_x + delta_track, t.min, t.max);
            float saved = arm.base_x;
            arm.base_x = new_x;
            if (joints_hit_tiles(arm, tiles)) arm.base_x = saved;
        } else {
            float new_y = std::clamp(arm.base_y + delta_track, t.min, t.max);
            float saved = arm.base_y;
            arm.base_y = new_y;
            if (joints_hit_tiles(arm, tiles)) arm.base_y = saved;
        }
        return;
    }

    int idx = arm.active_segment;
    Segment& seg = arm.segments[idx];

    if (seg.type == SegmentType::PIVOT || seg.type == SegmentType::BOTH) {
        float safe = clamp_segment_delta(arm, idx, true, delta_angle, tiles, all_arms, moving_arm_idx);
        seg.angle += safe;
    }
    if (seg.type == SegmentType::EXTEND || seg.type == SegmentType::BOTH) {
        float safe = clamp_segment_delta(arm, idx, false, delta_extend, tiles, all_arms, moving_arm_idx);
        // clamp_segment_delta's internal search also applies MIN_SEG_LEN to test geometry,
        // but it returns a raw delta — apply the floor here on the real length too.
        seg.length = std::max(MIN_SEG_LEN, seg.length + safe);
    }
}
