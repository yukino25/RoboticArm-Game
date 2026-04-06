// Game/types.h
#pragma once
#include <vector>
#include <optional>

#define GAME_WIDTH  24U
#define GAME_HEIGHT 18U

constexpr float BLOCK_SIZE    = 24.0f;
constexpr float GRAVITY       = 500.0f;   // px/s²
constexpr float PIVOT_SPEED   = 2.0f;     // rad/s
constexpr float EXTEND_SPEED  = 80.0f;    // px/s
constexpr float TRACK_SPEED   = 100.0f;   // px/s
constexpr float GRAB_RADIUS   = 12.0f;    // px
constexpr float MIN_SEG_LEN   = 10.0f;    // px
constexpr float OBJ_W                        = 20.0f;    // px
constexpr float OBJ_H                        = 20.0f;    // px
constexpr int   SEGMENT_COLLISION_ITERATIONS = 8;       // binary search passes per frame
constexpr float ARM_RADIUS                   = 8.0f;    // min distance between joints of different arms (px)

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
