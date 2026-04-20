// Game/config.h
// Tunable game constants — edit here to adjust gameplay feel and physics.
#pragma once

// ---- Grid ----------------------------------------------------------------
#define GAME_WIDTH  24U   // tiles per row
#define GAME_HEIGHT 18U   // tiles per column
constexpr float BLOCK_SIZE   = 32.0f;   // px per tile

// ---- Physics -------------------------------------------------------------
constexpr float GRAVITY      = 500.0f;  // px/s²

// ---- Arm movement --------------------------------------------------------
constexpr float PIVOT_SPEED  = 7.0f;    // rad/s (at MIN_SEG_LEN; longer segments rotate slower)
constexpr float EXTEND_SPEED = 80.0f;   // px/s
constexpr float TRACK_SPEED  = 100.0f;  // px/s
constexpr float MIN_SEG_LEN  = 10.0f;   // px — safety floor (per-segment max_length governs actual limit)

// ---- Grab / interaction --------------------------------------------------
constexpr float GRAB_RADIUS  = 12.0f;   // px — how close the tip must be to grab an object

// ---- Object --------------------------------------------------------------
constexpr float OBJ_W        = 20.0f;   // px
constexpr float OBJ_H        = 20.0f;   // px

// ---- Collision -----------------------------------------------------------
constexpr int   SEGMENT_COLLISION_ITERATIONS = 8;     // binary-search passes per frame
constexpr float ARM_RADIUS                   = 8.0f;  // px — min joint-to-joint distance between arms
