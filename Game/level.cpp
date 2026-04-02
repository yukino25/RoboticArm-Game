// Game/level.cpp
#include "level.h"
#include <algorithm>

Level make_level_1() {
    Level level;
    std::fill(std::begin(level.tiles), std::end(level.tiles), TileType::EMPTY);

    // Floor at row 17
    for (int x = 0; x < (int)GAME_WIDTH; x++)
        level.tiles[17 * GAME_WIDTH + x] = TileType::SOLID;

    // Left wall at column 0
    for (int y = 0; y < (int)GAME_HEIGHT; y++)
        level.tiles[y * GAME_WIDTH + 0] = TileType::SOLID;

    // Platform at row 13, columns 8-16
    for (int x = 8; x <= 16; x++)
        level.tiles[13 * GAME_WIDTH + x] = TileType::SOLID;

    // Arm: segment 0 = PIVOT, segment 1 = EXTEND
    Arm arm;
    arm.base_x = 1 * BLOCK_SIZE;
    arm.base_y = 9 * BLOCK_SIZE;
    arm.base_angle = 0.0f;
    arm.active_segment = 0;

    arm.segments.push_back({SegmentType::PIVOT,  0.0f, 6 * BLOCK_SIZE});
    arm.segments.push_back({SegmentType::EXTEND, 0.0f, 3 * BLOCK_SIZE});

    level.arms.push_back(arm);
    level.active_arm = 0;

    // Object resting on platform
    level.object = {
        12 * BLOCK_SIZE,
        13 * BLOCK_SIZE - OBJ_H,
        0.0f, 0.0f, false
    };

    // Target zone on the floor to the right
    level.target_zone = {18 * BLOCK_SIZE, 16 * BLOCK_SIZE, 3 * BLOCK_SIZE, BLOCK_SIZE};

    return level;
}
