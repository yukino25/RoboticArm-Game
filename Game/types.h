// Game/types.h
#pragma once
#include "config.h"
#include <vector>
#include <optional>

struct Vec2 { float x, y; };
struct Rect { float x, y, w, h; };

enum class SegmentType { PIVOT, EXTEND, BOTH };
enum class TileType    { EMPTY, SOLID };

struct Segment {
    SegmentType type;
    float angle;   // radians, relative to parent segment
    float length;  // pixels
};

struct Track {
    float min, max;
    bool horizontal; // true = slides on X, false = on Y
};

struct Arm {
    std::vector<Segment> segments;
    float base_x, base_y;
    float base_angle;
    std::optional<Track> track;
    int active_segment; // -1 = base selected
};

struct Object {
    float x, y;
    float vx, vy;
    bool grabbed;
};

struct Level {
    TileType tiles[GAME_WIDTH * GAME_HEIGHT];
    std::vector<Arm> arms;
    Object object;
    Rect target_zone;
    int active_arm;
};

struct GameState {
    Level level;
    bool won;
    int level_index;
};
