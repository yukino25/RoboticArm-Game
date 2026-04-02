// Game/movement.h
#pragma once
#include "types.h"

// direction: +1 = forward (key 2), -1 = backward (key 1)
// Cycles (active_arm, active_segment) through all segments of all arms.
// active_segment == -1 means base selected (only for track arms).
void cycle_selection(GameState& gs, int direction);