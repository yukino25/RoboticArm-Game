// Game/movement.h
#pragma once
#include "types.h"
#include <vector>

// direction: +1 = forward (key 2), -1 = backward (key 1)
// Cycles (active_arm, active_segment) through all segments of all arms.
// active_segment == -1 means base selected (only for track arms).
void cycle_selection(GameState& gs, int direction);

// Apply one frame of input to the selected segment/base.
// delta_angle:  radians (for PIVOT or BOTH)
// delta_extend: pixels  (for EXTEND or BOTH)
// delta_track:  pixels  (for base on track when active_segment == -1)
void apply_movement(Arm& arm, float delta_angle, float delta_extend, float delta_track);

// Returns the largest safe delta in [0, |requested_delta|] (preserving sign)
// such that all joints of `arm` remain clear of solid tiles and away from
// joints of other arms. Pass moving_arm_idx to exclude `arm` from arm-to-arm check.
float clamp_segment_delta(
    Arm arm,                          // copy — mutated during binary search
    int seg_idx,                      // segment index being moved (-1 = not a segment move)
    bool is_angle,                    // true = rotating, false = extending
    float requested_delta,
    const TileType* tiles,
    const std::vector<Arm>& all_arms,
    int moving_arm_idx
);