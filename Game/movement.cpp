// Game/movement.cpp
#include "movement.h"

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