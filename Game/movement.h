// Game/movement.h
#pragma once
#include "types.h"

// direction: +1 = forward (key 2), -1 = backward (key 1)
// Cycles (active_arm, active_segment) through all segments of all arms.
// active_segment == -1 means base selected (only for track arms).
void cycle_selection(GameState& gs, int direction);

// Apply one frame of input to the selected segment/base.
// delta_angle:  radians (for PIVOT or BOTH)
// delta_extend: pixels  (for EXTEND or BOTH)
// delta_track:  pixels  (for base on track when active_segment == -1)
void apply_movement(Arm& arm, float delta_angle, float delta_extend, float delta_track);