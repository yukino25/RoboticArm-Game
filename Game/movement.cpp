// Game/movement.cpp
#include "movement.h"
#include "gravity.h"
#include <algorithm>

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

    float lo = 0.0f;
    float hi = requested_delta;

    // Tile collision binary search
    for (int i = 0; i < SEGMENT_COLLISION_ITERATIONS; i++) {
        float mid = (lo + hi) / 2.0f;
        Arm test = arm;
        if (seg_idx >= 0 && seg_idx < (int)test.segments.size()) {
            if (is_angle) test.segments[seg_idx].angle  += mid;
            else          test.segments[seg_idx].length  = std::max(MIN_SEG_LEN, test.segments[seg_idx].length + mid);
        }
        if (joints_hit_tiles(test, tiles)) hi = mid;
        else                               lo = mid;
    }

    // Arm-to-arm collision binary search (lo so far is tile-safe)
    float arm_hi = lo;
    float arm_lo = 0.0f;
    for (int i = 0; i < SEGMENT_COLLISION_ITERATIONS; i++) {
        float mid = (arm_lo + arm_hi) / 2.0f;
        Arm test = arm;
        if (seg_idx >= 0 && seg_idx < (int)test.segments.size()) {
            if (is_angle) test.segments[seg_idx].angle  += mid;
            else          test.segments[seg_idx].length  = std::max(MIN_SEG_LEN, test.segments[seg_idx].length + mid);
        }
        auto joints_a = compute_fk(test);
        bool collides = false;
        for (int ai = 0; ai < (int)all_arms.size() && !collides; ai++) {
            if (ai == moving_arm_idx) continue;
            auto joints_b = compute_fk(all_arms[ai]);
            for (const auto& ja : joints_a) {
                for (const auto& jb : joints_b) {
                    float dx = ja.x - jb.x, dy = ja.y - jb.y;
                    if (dx*dx + dy*dy < ARM_RADIUS * ARM_RADIUS) {
                        collides = true;
                        break;
                    }
                }
                if (collides) break;
            }
        }
        if (collides) arm_hi = mid;
        else          arm_lo = mid;
    }

    return arm_lo;
}

void apply_movement(Arm& arm, float delta_angle, float delta_extend, float delta_track) {
    if (arm.active_segment == -1) {
        if (!arm.track.has_value()) return;
        const Track& t = arm.track.value();
        if (t.horizontal) {
            arm.base_x = std::clamp(arm.base_x + delta_track, t.min, t.max);
        } else {
            arm.base_y = std::clamp(arm.base_y + delta_track, t.min, t.max);
        }
        return;
    }

    Segment& seg = arm.segments[arm.active_segment];

    if (seg.type == SegmentType::PIVOT || seg.type == SegmentType::BOTH) {
        seg.angle += delta_angle;
    }
    if (seg.type == SegmentType::EXTEND || seg.type == SegmentType::BOTH) {
        seg.length = std::max(MIN_SEG_LEN, seg.length + delta_extend);
    }
}