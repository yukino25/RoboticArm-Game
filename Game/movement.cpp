// Game/movement.cpp
#include "movement.h"
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