# Robotic Arm Game Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build Phase A of a 2D side-scrolling robotic-arm puzzle game in C++/SDL3 — player controls an arm's pivot/extend segments to grab an object and place it in a target zone.

**Architecture:** The arm is a kinematic chain; joint positions are computed each frame via forward kinematics (no physics on the arm itself). The grabbed object runs simple Euler gravity + AABB tile collision. `gravity.cpp` owns all physics math, `movement.cpp` owns all input logic; both are SDL-free so they can be unit-tested without SDL.

**Tech Stack:** C++17, SDL3 (windowing/rendering/input), Catch2 v3 (unit tests via CMake FetchContent)

---

## File Map

| File | Responsibility |
|------|---------------|
| `Game/types.h` | All data structures and numeric constants — SDL-free |
| `Game/gravity.h` / `gravity.cpp` | FK chain, object physics, tile collision, grab logic — SDL-free |
| `Game/movement.h` / `movement.cpp` | Selection cursor cycling, arm segment mutation — SDL-free |
| `Game/level.h` / `level.cpp` | Level factory functions |
| `Game/render.h` / `render.cpp` | SDL3 drawing — only file that touches SDL types |
| `Game/main.cpp` | SDL3 app callbacks; wires all systems together |
| `Game/tests/test_physics.cpp` | Catch2 tests for gravity.cpp |
| `Game/tests/test_movement.cpp` | Catch2 tests for movement.cpp |
| `Game/CMakeLists.txt` | Build config: `game` target + `tests` target + Catch2 FetchContent |

---

## Task 1: Project Structure — types.h, CMakeLists.txt, empty stubs

**Files:**
- Create: `Game/types.h`
- Modify: `Game/CMakeLists.txt`
- Create: `Game/tests/test_physics.cpp` (empty scaffold)
- Create: `Game/tests/test_movement.cpp` (empty scaffold)

- [ ] **Step 1: Write types.h**

```cpp
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
constexpr float OBJ_W         = 20.0f;    // px
constexpr float OBJ_H         = 20.0f;    // px

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
```

- [ ] **Step 2: Rewrite CMakeLists.txt**

```cmake
cmake_minimum_required(VERSION 3.16)
project(RoboticArmGame)

set(CMAKE_CXX_STANDARD 17)

add_subdirectory(SDL)

# Game executable
add_executable(game
    main.cpp
    gravity.cpp
    movement.cpp
    level.cpp
    render.cpp
)
target_include_directories(game PRIVATE .)
target_link_libraries(game PRIVATE SDL3::SDL3)

# Unit tests (no SDL dependency)
include(FetchContent)
FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.4.0
)
FetchContent_MakeAvailable(Catch2)

add_executable(tests
    tests/test_physics.cpp
    tests/test_movement.cpp
    gravity.cpp
    movement.cpp
)
target_include_directories(tests PRIVATE .)
target_link_libraries(tests PRIVATE Catch2::Catch2WithMain)
```

- [ ] **Step 3: Write empty test scaffolds**

`Game/tests/test_physics.cpp`:
```cpp
#include <catch2/catch_test_macros.hpp>
#include "../types.h"
#include "../gravity.h"
// Tests added in Tasks 2-5
```

`Game/tests/test_movement.cpp`:
```cpp
#include <catch2/catch_test_macros.hpp>
#include "../types.h"
#include "../movement.h"
// Tests added in Tasks 6-7
```

- [ ] **Step 4: Verify CMake configures (Catch2 downloads)**

```bash
cd /Users/nickschlosser/RoboticArm-Game/Game
cmake -B build -S .
```
Expected: configures without error; Catch2 cloned into `build/_deps/`

- [ ] **Step 5: Commit**

```bash
cd /Users/nickschlosser/RoboticArm-Game
git add Game/types.h Game/CMakeLists.txt Game/tests/test_physics.cpp Game/tests/test_movement.cpp
git commit -m "feat: add types.h, test scaffolds, update CMakeLists for Catch2"
```

---

## Task 2: Forward Kinematics

**Files:**
- Modify: `Game/gravity.h`
- Modify: `Game/gravity.cpp`
- Modify: `Game/tests/test_physics.cpp`

- [ ] **Step 1: Declare FK functions in gravity.h**

```cpp
// Game/gravity.h
#pragma once
#include "types.h"
#include <vector>

// Returns n+1 joint positions: joints[0]=base, joints[n]=tip
std::vector<Vec2> compute_fk(const Arm& arm);

// Convenience: returns arm tip position
Vec2 arm_tip(const Arm& arm);
```

- [ ] **Step 2: Write failing FK tests**

Add to `Game/tests/test_physics.cpp`:
```cpp
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <cmath>
#include "../types.h"
#include "../gravity.h"

static Arm make_arm(float base_x, float base_y, float base_angle) {
    Arm a;
    a.base_x = base_x; a.base_y = base_y; a.base_angle = base_angle;
    a.active_segment = 0;
    return a;
}

static Segment make_seg(SegmentType t, float angle, float length) {
    return {t, angle, length};
}

TEST_CASE("FK: single horizontal segment") {
    Arm arm = make_arm(100.0f, 200.0f, 0.0f);
    arm.segments.push_back(make_seg(SegmentType::PIVOT, 0.0f, 50.0f));

    auto joints = compute_fk(arm);

    REQUIRE(joints.size() == 2);
    REQUIRE_THAT(joints[0].x, Catch::Matchers::WithinAbs(100.0f, 0.001f));
    REQUIRE_THAT(joints[0].y, Catch::Matchers::WithinAbs(200.0f, 0.001f));
    REQUIRE_THAT(joints[1].x, Catch::Matchers::WithinAbs(150.0f, 0.001f));
    REQUIRE_THAT(joints[1].y, Catch::Matchers::WithinAbs(200.0f, 0.001f));
}

TEST_CASE("FK: two segments with right-angle turn") {
    // seg0 points right (angle=0, len=50)
    // seg1 relative angle=PI/2 so absolute=PI/2, points down (len=30)
    Arm arm = make_arm(0.0f, 0.0f, 0.0f);
    arm.segments.push_back(make_seg(SegmentType::PIVOT, 0.0f, 50.0f));
    arm.segments.push_back(make_seg(SegmentType::PIVOT, float(M_PI / 2), 30.0f));

    auto joints = compute_fk(arm);

    REQUIRE(joints.size() == 3);
    REQUIRE_THAT(joints[1].x, Catch::Matchers::WithinAbs(50.0f,  0.001f));
    REQUIRE_THAT(joints[1].y, Catch::Matchers::WithinAbs(0.0f,   0.001f));
    REQUIRE_THAT(joints[2].x, Catch::Matchers::WithinAbs(50.0f,  0.001f));
    REQUIRE_THAT(joints[2].y, Catch::Matchers::WithinAbs(30.0f,  0.001f));
}

TEST_CASE("FK: base_angle offsets all segments") {
    // base_angle = PI/2, single horizontal segment → should point down
    Arm arm = make_arm(0.0f, 0.0f, float(M_PI / 2));
    arm.segments.push_back(make_seg(SegmentType::PIVOT, 0.0f, 40.0f));

    auto joints = compute_fk(arm);

    REQUIRE_THAT(joints[1].x, Catch::Matchers::WithinAbs(0.0f,  0.001f));
    REQUIRE_THAT(joints[1].y, Catch::Matchers::WithinAbs(40.0f, 0.001f));
}

TEST_CASE("arm_tip returns last FK joint") {
    Arm arm = make_arm(10.0f, 20.0f, 0.0f);
    arm.segments.push_back(make_seg(SegmentType::PIVOT, 0.0f, 100.0f));

    Vec2 tip = arm_tip(arm);
    REQUIRE_THAT(tip.x, Catch::Matchers::WithinAbs(110.0f, 0.001f));
    REQUIRE_THAT(tip.y, Catch::Matchers::WithinAbs(20.0f,  0.001f));
}
```

- [ ] **Step 3: Run tests — verify they fail**

```bash
cd /Users/nickschlosser/RoboticArm-Game/Game
cmake --build build --target tests 2>&1 | tail -5
```
Expected: compile error (compute_fk undefined)

- [ ] **Step 4: Implement FK in gravity.cpp**

```cpp
// Game/gravity.cpp
#include "gravity.h"
#include <cmath>
#include <algorithm>

std::vector<Vec2> compute_fk(const Arm& arm) {
    std::vector<Vec2> joints;
    joints.reserve(arm.segments.size() + 1);
    joints.push_back({arm.base_x, arm.base_y});

    float cumulative = arm.base_angle;
    for (const auto& seg : arm.segments) {
        cumulative += seg.angle;
        const Vec2& prev = joints.back();
        joints.push_back({
            prev.x + std::cos(cumulative) * seg.length,
            prev.y + std::sin(cumulative) * seg.length
        });
    }
    return joints;
}

Vec2 arm_tip(const Arm& arm) {
    return compute_fk(arm).back();
}
```

- [ ] **Step 5: Run tests — verify they pass**

```bash
cd /Users/nickschlosser/RoboticArm-Game/Game
cmake --build build --target tests && ./build/tests "[FK]"
```
Wait — Catch2 auto-registers by test name. Run all:
```bash
./build/tests
```
Expected: 4 tests pass

- [ ] **Step 6: Commit**

```bash
cd /Users/nickschlosser/RoboticArm-Game
git add Game/gravity.h Game/gravity.cpp Game/tests/test_physics.cpp
git commit -m "feat: implement forward kinematics chain"
```

---

## Task 3: Object Physics

**Files:**
- Modify: `Game/gravity.h`
- Modify: `Game/gravity.cpp`
- Modify: `Game/tests/test_physics.cpp`

- [ ] **Step 1: Declare update_object in gravity.h**

Add below existing declarations:
```cpp
// Integrate gravity + velocity for one frame. Does nothing if obj.grabbed.
void update_object(Object& obj, float dt);
```

- [ ] **Step 2: Write failing tests**

Append to `Game/tests/test_physics.cpp`:
```cpp
TEST_CASE("update_object: applies gravity and integrates position") {
    Object obj{100.0f, 100.0f, 0.0f, 0.0f, false};

    update_object(obj, 1.0f); // 1 second

    REQUIRE_THAT(obj.vy, Catch::Matchers::WithinAbs(GRAVITY, 0.001f));
    REQUIRE_THAT(obj.y,  Catch::Matchers::WithinAbs(100.0f + GRAVITY, 0.001f));
}

TEST_CASE("update_object: horizontal velocity integrates") {
    Object obj{100.0f, 0.0f, 50.0f, 0.0f, false};

    update_object(obj, 0.1f);

    REQUIRE_THAT(obj.x, Catch::Matchers::WithinAbs(105.0f, 0.001f));
}

TEST_CASE("update_object: does nothing when grabbed") {
    Object obj{100.0f, 100.0f, 0.0f, 0.0f, true};

    update_object(obj, 1.0f);

    REQUIRE_THAT(obj.y,  Catch::Matchers::WithinAbs(100.0f, 0.001f));
    REQUIRE_THAT(obj.vy, Catch::Matchers::WithinAbs(0.0f,   0.001f));
}
```

- [ ] **Step 3: Run tests — verify new ones fail**

```bash
cd /Users/nickschlosser/RoboticArm-Game/Game
cmake --build build --target tests && ./build/tests
```
Expected: 3 new failures (update_object undefined)

- [ ] **Step 4: Implement update_object in gravity.cpp**

Append to `Game/gravity.cpp`:
```cpp
void update_object(Object& obj, float dt) {
    if (obj.grabbed) return;
    obj.vy += GRAVITY * dt;
    obj.x  += obj.vx * dt;
    obj.y  += obj.vy * dt;

    // Clamp to world bounds
    if (obj.x < 0.0f)                           { obj.x = 0.0f;                                   obj.vx = 0.0f; }
    if (obj.y < 0.0f)                           { obj.y = 0.0f;                                   obj.vy = 0.0f; }
    if (obj.x + OBJ_W > GAME_WIDTH  * BLOCK_SIZE) { obj.x = GAME_WIDTH  * BLOCK_SIZE - OBJ_W; obj.vx = 0.0f; }
    if (obj.y + OBJ_H > GAME_HEIGHT * BLOCK_SIZE) { obj.y = GAME_HEIGHT * BLOCK_SIZE - OBJ_H; obj.vy = 0.0f; }
}
```

- [ ] **Step 5: Run tests — all pass**

```bash
./build/tests
```
Expected: all tests pass

- [ ] **Step 6: Commit**

```bash
cd /Users/nickschlosser/RoboticArm-Game
git add Game/gravity.h Game/gravity.cpp Game/tests/test_physics.cpp
git commit -m "feat: implement object gravity and world-bounds clamping"
```

---

## Task 4: Tile Collision

**Files:**
- Modify: `Game/gravity.h`
- Modify: `Game/gravity.cpp`
- Modify: `Game/tests/test_physics.cpp`

- [ ] **Step 1: Declare resolve_tile_collision in gravity.h**

```cpp
// Resolve object AABB against solid tiles. Modifies obj pos/vel.
void resolve_tile_collision(Object& obj, const TileType* tiles, float obj_w, float obj_h);
```

- [ ] **Step 2: Write failing tests**

Append to `Game/tests/test_physics.cpp`:
```cpp
static void fill_floor(TileType* tiles, int row) {
    for (int x = 0; x < (int)GAME_WIDTH; x++)
        tiles[row * GAME_WIDTH + x] = TileType::SOLID;
}

TEST_CASE("tile collision: object stops on solid floor") {
    TileType tiles[GAME_WIDTH * GAME_HEIGHT];
    std::fill(tiles, tiles + GAME_WIDTH * GAME_HEIGHT, TileType::EMPTY);
    fill_floor(tiles, 5); // floor at y = 5*24 = 120

    // Object overlaps floor from below
    Object obj{48.0f, 5 * BLOCK_SIZE - 10.0f, 0.0f, 200.0f, false};

    resolve_tile_collision(obj, tiles, OBJ_W, OBJ_H);

    // Bottom of object should rest on top of floor tile
    REQUIRE_THAT(obj.y,  Catch::Matchers::WithinAbs(5 * BLOCK_SIZE - OBJ_H, 0.5f));
    REQUIRE_THAT(obj.vy, Catch::Matchers::WithinAbs(0.0f, 0.001f));
}

TEST_CASE("tile collision: object stops against left wall") {
    TileType tiles[GAME_WIDTH * GAME_HEIGHT];
    std::fill(tiles, tiles + GAME_WIDTH * GAME_HEIGHT, TileType::EMPTY);
    // Vertical wall at column 3
    for (int y = 0; y < (int)GAME_HEIGHT; y++)
        tiles[y * GAME_WIDTH + 3] = TileType::SOLID;

    // Object moving right into the wall
    Object obj{3 * BLOCK_SIZE - 5.0f, 48.0f, 100.0f, 0.0f, false};

    resolve_tile_collision(obj, tiles, OBJ_W, OBJ_H);

    REQUIRE_THAT(obj.x,  Catch::Matchers::WithinAbs(3 * BLOCK_SIZE - OBJ_W, 0.5f));
    REQUIRE_THAT(obj.vx, Catch::Matchers::WithinAbs(0.0f, 0.001f));
}

TEST_CASE("tile collision: no change when object in empty space") {
    TileType tiles[GAME_WIDTH * GAME_HEIGHT];
    std::fill(tiles, tiles + GAME_WIDTH * GAME_HEIGHT, TileType::EMPTY);

    Object obj{100.0f, 100.0f, 50.0f, 50.0f, false};
    resolve_tile_collision(obj, tiles, OBJ_W, OBJ_H);

    REQUIRE_THAT(obj.x,  Catch::Matchers::WithinAbs(100.0f, 0.001f));
    REQUIRE_THAT(obj.y,  Catch::Matchers::WithinAbs(100.0f, 0.001f));
}
```

- [ ] **Step 3: Run tests — verify new ones fail**

```bash
cd /Users/nickschlosser/RoboticArm-Game/Game && cmake --build build --target tests && ./build/tests
```
Expected: 3 new failures

- [ ] **Step 4: Implement resolve_tile_collision in gravity.cpp**

Append to `Game/gravity.cpp`:
```cpp
void resolve_tile_collision(Object& obj, const TileType* tiles, float obj_w, float obj_h) {
    for (int iter = 0; iter < 3; iter++) { // up to 3 passes for corner cases
        float left   = obj.x;
        float top    = obj.y;
        float right  = obj.x + obj_w;
        float bottom = obj.y + obj_h;

        int tx0 = std::max(0, (int)(left   / BLOCK_SIZE));
        int ty0 = std::max(0, (int)(top    / BLOCK_SIZE));
        int tx1 = std::min((int)GAME_WIDTH  - 1, (int)((right  - 1.0f) / BLOCK_SIZE));
        int ty1 = std::min((int)GAME_HEIGHT - 1, (int)((bottom - 1.0f) / BLOCK_SIZE));

        bool any = false;
        for (int ty = ty0; ty <= ty1; ty++) {
            for (int tx = tx0; tx <= tx1; tx++) {
                if (tiles[ty * GAME_WIDTH + tx] != TileType::SOLID) continue;

                float tile_x = tx * BLOCK_SIZE;
                float tile_y = ty * BLOCK_SIZE;

                float ov_left   = right  - tile_x;
                float ov_right  = tile_x + BLOCK_SIZE - left;
                float ov_top    = bottom - tile_y;
                float ov_bottom = tile_y + BLOCK_SIZE - top;

                float min_x = std::min(ov_left, ov_right);
                float min_y = std::min(ov_top,  ov_bottom);

                if (min_x < min_y) {
                    if (ov_left < ov_right) { obj.x -= ov_left;   obj.vx = std::min(0.0f, obj.vx); }
                    else                    { obj.x += ov_right;  obj.vx = std::max(0.0f, obj.vx); }
                } else {
                    if (ov_top < ov_bottom) { obj.y -= ov_top;    obj.vy = std::min(0.0f, obj.vy); }
                    else                    { obj.y += ov_bottom; obj.vy = std::max(0.0f, obj.vy); }
                }
                any = true;
            }
        }
        if (!any) break;
    }
}
```

- [ ] **Step 5: Run all tests**

```bash
./build/tests
```
Expected: all tests pass

- [ ] **Step 6: Commit**

```bash
cd /Users/nickschlosser/RoboticArm-Game
git add Game/gravity.h Game/gravity.cpp Game/tests/test_physics.cpp
git commit -m "feat: implement AABB tile collision resolution"
```

---

## Task 5: Grab Mechanics

**Files:**
- Modify: `Game/gravity.h`
- Modify: `Game/gravity.cpp`
- Modify: `Game/tests/test_physics.cpp`

- [ ] **Step 1: Declare grab functions in gravity.h**

```cpp
// True when object center is within GRAB_RADIUS of arm tip
bool can_grab(const Object& obj, const Arm& arm);

// Set grabbed=true, zero velocity (position snapped to tip each frame in main)
void grab_object(Object& obj);

// Set grabbed=false, inherit tip velocity
void release_object(Object& obj, Vec2 tip_vel);
```

- [ ] **Step 2: Write failing tests**

Append to `Game/tests/test_physics.cpp`:
```cpp
static Arm make_horizontal_arm(float base_x, float base_y, float length) {
    Arm arm = make_arm(base_x, base_y, 0.0f);
    arm.segments.push_back(make_seg(SegmentType::PIVOT, 0.0f, length));
    return arm;
}

TEST_CASE("can_grab: true when object center is within GRAB_RADIUS of tip") {
    // Tip at (100, 0). Object center at (95, 0) → distance 5 < 12
    Arm arm = make_horizontal_arm(0.0f, 0.0f, 100.0f);
    Object obj{95.0f - OBJ_W / 2, -OBJ_H / 2, 0.0f, 0.0f, false};

    REQUIRE(can_grab(obj, arm) == true);
}

TEST_CASE("can_grab: false when object center is beyond GRAB_RADIUS") {
    // Tip at (100, 0). Object center at (115, 0) → distance 15 > 12
    Arm arm = make_horizontal_arm(0.0f, 0.0f, 100.0f);
    Object obj{115.0f - OBJ_W / 2, -OBJ_H / 2, 0.0f, 0.0f, false};

    REQUIRE(can_grab(obj, arm) == false);
}

TEST_CASE("grab_object: sets grabbed=true and zeros velocity") {
    Object obj{0.0f, 0.0f, 30.0f, 100.0f, false};
    grab_object(obj);

    REQUIRE(obj.grabbed == true);
    REQUIRE_THAT(obj.vx, Catch::Matchers::WithinAbs(0.0f, 0.001f));
    REQUIRE_THAT(obj.vy, Catch::Matchers::WithinAbs(0.0f, 0.001f));
}

TEST_CASE("release_object: sets grabbed=false and inherits tip velocity") {
    Object obj{0.0f, 0.0f, 0.0f, 0.0f, true};
    Vec2 tip_vel{150.0f, -80.0f};
    release_object(obj, tip_vel);

    REQUIRE(obj.grabbed == false);
    REQUIRE_THAT(obj.vx, Catch::Matchers::WithinAbs(150.0f, 0.001f));
    REQUIRE_THAT(obj.vy, Catch::Matchers::WithinAbs(-80.0f, 0.001f));
}
```

- [ ] **Step 3: Run tests — verify new ones fail**

```bash
cd /Users/nickschlosser/RoboticArm-Game/Game && cmake --build build --target tests && ./build/tests
```
Expected: 4 new failures

- [ ] **Step 4: Implement grab functions in gravity.cpp**

Append to `Game/gravity.cpp`:
```cpp
bool can_grab(const Object& obj, const Arm& arm) {
    Vec2 tip = arm_tip(arm);
    float cx = obj.x + OBJ_W / 2;
    float cy = obj.y + OBJ_H / 2;
    float dx = tip.x - cx;
    float dy = tip.y - cy;
    return dx * dx + dy * dy <= GRAB_RADIUS * GRAB_RADIUS;
}

void grab_object(Object& obj) {
    obj.grabbed = true;
    obj.vx = 0.0f;
    obj.vy = 0.0f;
}

void release_object(Object& obj, Vec2 tip_vel) {
    obj.grabbed = false;
    obj.vx = tip_vel.x;
    obj.vy = tip_vel.y;
}
```

- [ ] **Step 5: Run all tests**

```bash
./build/tests
```
Expected: all tests pass

- [ ] **Step 6: Commit**

```bash
cd /Users/nickschlosser/RoboticArm-Game
git add Game/gravity.h Game/gravity.cpp Game/tests/test_physics.cpp
git commit -m "feat: implement grab, release, and grab-detection"
```

---

## Task 6: Selection Cursor

**Files:**
- Modify: `Game/movement.h`
- Modify: `Game/movement.cpp`
- Modify: `Game/tests/test_movement.cpp`

- [ ] **Step 1: Declare cycle_selection in movement.h**

```cpp
// Game/movement.h
#pragma once
#include "types.h"

// direction: +1 = forward (key 2), -1 = backward (key 1)
// Cycles (arm_index, active_segment) through all segments of all arms.
// active_segment == -1 means base selected (only meaningful for track arms).
void cycle_selection(GameState& gs, int direction);
```

- [ ] **Step 2: Write failing tests**

Replace contents of `Game/tests/test_movement.cpp`:
```cpp
#include <catch2/catch_test_macros.hpp>
#include <algorithm>
#include "../types.h"
#include "../movement.h"

static GameState make_gs_one_arm(int num_segments, bool has_track = false) {
    GameState gs{};
    gs.won = false;
    gs.level_index = 0;

    Level& level = gs.level;
    std::fill(std::begin(level.tiles), std::end(level.tiles), TileType::EMPTY);
    level.active_arm = 0;
    level.target_zone = {0, 0, 10, 10};
    level.object = {0, 0, 0, 0, false};

    Arm arm;
    arm.base_x = 0; arm.base_y = 0; arm.base_angle = 0;
    arm.active_segment = -1; // start at base
    if (has_track) arm.track = Track{0.0f, 100.0f, true};
    for (int i = 0; i < num_segments; i++) {
        Segment s{SegmentType::PIVOT, 0.0f, 50.0f};
        arm.segments.push_back(s);
    }
    level.arms.push_back(arm);
    return gs;
}

TEST_CASE("cycle_selection: forward moves base→seg0→seg1") {
    GameState gs = make_gs_one_arm(2, /*has_track=*/true);
    REQUIRE(gs.level.arms[0].active_segment == -1);

    cycle_selection(gs, +1);
    REQUIRE(gs.level.arms[0].active_segment == 0);

    cycle_selection(gs, +1);
    REQUIRE(gs.level.arms[0].active_segment == 1);
}

TEST_CASE("cycle_selection: forward wraps last segment back to base") {
    GameState gs = make_gs_one_arm(2, true);
    gs.level.arms[0].active_segment = 1; // at last segment

    cycle_selection(gs, +1);
    REQUIRE(gs.level.arms[0].active_segment == -1);
}

TEST_CASE("cycle_selection: backward moves seg0 to base") {
    GameState gs = make_gs_one_arm(2, true);
    gs.level.arms[0].active_segment = 0;

    cycle_selection(gs, -1);
    REQUIRE(gs.level.arms[0].active_segment == -1);
}

TEST_CASE("cycle_selection: crosses from arm0 last seg to arm1 base") {
    GameState gs = make_gs_one_arm(2);
    // Add second arm
    Arm arm2;
    arm2.base_x = 0; arm2.base_y = 0; arm2.base_angle = 0;
    arm2.active_segment = -1;
    arm2.track = Track{0.0f, 100.0f, true};
    arm2.segments.push_back({SegmentType::PIVOT, 0.0f, 50.0f});
    gs.level.arms.push_back(arm2);

    gs.level.arms[0].active_segment = 1; // last seg of arm0

    cycle_selection(gs, +1);

    REQUIRE(gs.level.active_arm == 1);
    REQUIRE(gs.level.arms[1].active_segment == -1);
}

TEST_CASE("cycle_selection: no base for arm without track (skips -1)") {
    GameState gs = make_gs_one_arm(2, /*has_track=*/false);
    // arm has no track so -1 should be skipped; start should land on seg 0
    gs.level.arms[0].active_segment = 1;

    cycle_selection(gs, +1); // wrap: should go to seg 0 (skipping base)
    REQUIRE(gs.level.arms[0].active_segment == 0);
}
```

- [ ] **Step 3: Run tests — verify they fail**

```bash
cd /Users/nickschlosser/RoboticArm-Game/Game && cmake --build build --target tests && ./build/tests
```
Expected: compile errors (cycle_selection undefined)

- [ ] **Step 4: Implement cycle_selection in movement.cpp**

```cpp
// Game/movement.cpp
#include "movement.h"
#include <algorithm>

void cycle_selection(GameState& gs, int direction) {
    Level& level = gs.level;
    int num_arms = (int)level.arms.size();
    Arm& arm = level.arms[level.active_arm];
    int n = (int)arm.segments.size();
    bool has_base = arm.track.has_value();

    // Compute min index: -1 if track exists, else 0
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
```

- [ ] **Step 5: Run all tests**

```bash
./build/tests
```
Expected: all tests pass

- [ ] **Step 6: Commit**

```bash
cd /Users/nickschlosser/RoboticArm-Game
git add Game/movement.h Game/movement.cpp Game/tests/test_movement.cpp
git commit -m "feat: implement selection cursor cycling across segments and arms"
```

---

## Task 7: Arm Movement

**Files:**
- Modify: `Game/movement.h`
- Modify: `Game/movement.cpp`
- Modify: `Game/tests/test_movement.cpp`

- [ ] **Step 1: Declare apply_movement in movement.h**

Add below `cycle_selection`:
```cpp
// Apply one frame of input to the selected segment/base.
// delta_angle:  radians (for PIVOT or BOTH; ignored otherwise)
// delta_extend: pixels  (for EXTEND or BOTH; ignored otherwise)
// delta_track:  pixels  (for base on track; ignored if active_segment != -1)
void apply_movement(Arm& arm, float delta_angle, float delta_extend, float delta_track);
```

- [ ] **Step 2: Write failing tests**

Append to `Game/tests/test_movement.cpp`:
```cpp
static Arm make_simple_arm(SegmentType type, float initial_length = 50.0f) {
    Arm arm;
    arm.base_x = 0; arm.base_y = 0; arm.base_angle = 0;
    arm.active_segment = 0;
    arm.segments.push_back({type, 0.0f, initial_length});
    return arm;
}

TEST_CASE("apply_movement: rotates PIVOT segment") {
    Arm arm = make_simple_arm(SegmentType::PIVOT);

    apply_movement(arm, 0.5f, 0.0f, 0.0f);

    REQUIRE_THAT(arm.segments[0].angle, Catch::Matchers::WithinAbs(0.5f, 0.001f));
}

TEST_CASE("apply_movement: PIVOT ignores delta_extend") {
    Arm arm = make_simple_arm(SegmentType::PIVOT);

    apply_movement(arm, 0.0f, 20.0f, 0.0f);

    REQUIRE_THAT(arm.segments[0].length, Catch::Matchers::WithinAbs(50.0f, 0.001f));
}

TEST_CASE("apply_movement: extends EXTEND segment") {
    Arm arm = make_simple_arm(SegmentType::EXTEND);

    apply_movement(arm, 0.0f, 10.0f, 0.0f);

    REQUIRE_THAT(arm.segments[0].length, Catch::Matchers::WithinAbs(60.0f, 0.001f));
}

TEST_CASE("apply_movement: clamps length at MIN_SEG_LEN on retract") {
    Arm arm = make_simple_arm(SegmentType::EXTEND, 12.0f);

    apply_movement(arm, 0.0f, -100.0f, 0.0f); // retract way below min

    REQUIRE_THAT(arm.segments[0].length, Catch::Matchers::WithinAbs(MIN_SEG_LEN, 0.001f));
}

TEST_CASE("apply_movement: BOTH responds to both angle and length") {
    Arm arm = make_simple_arm(SegmentType::BOTH);

    apply_movement(arm, 1.0f, 5.0f, 0.0f);

    REQUIRE_THAT(arm.segments[0].angle,  Catch::Matchers::WithinAbs(1.0f,  0.001f));
    REQUIRE_THAT(arm.segments[0].length, Catch::Matchers::WithinAbs(55.0f, 0.001f));
}

TEST_CASE("apply_movement: base moves along horizontal track") {
    Arm arm;
    arm.base_x = 50.0f; arm.base_y = 0.0f; arm.base_angle = 0.0f;
    arm.active_segment = -1;
    arm.track = Track{0.0f, 200.0f, true};
    arm.segments.push_back({SegmentType::PIVOT, 0.0f, 50.0f});

    apply_movement(arm, 0.0f, 0.0f, 20.0f);

    REQUIRE_THAT(arm.base_x, Catch::Matchers::WithinAbs(70.0f, 0.001f));
}

TEST_CASE("apply_movement: base clamps at track max") {
    Arm arm;
    arm.base_x = 190.0f; arm.base_y = 0.0f; arm.base_angle = 0.0f;
    arm.active_segment = -1;
    arm.track = Track{0.0f, 200.0f, true};
    arm.segments.push_back({SegmentType::PIVOT, 0.0f, 50.0f});

    apply_movement(arm, 0.0f, 0.0f, 50.0f); // would overshoot

    REQUIRE_THAT(arm.base_x, Catch::Matchers::WithinAbs(200.0f, 0.001f));
}
```

- [ ] **Step 3: Run tests — verify new ones fail**

```bash
cd /Users/nickschlosser/RoboticArm-Game/Game && cmake --build build --target tests && ./build/tests
```
Expected: 7 new failures

- [ ] **Step 4: Implement apply_movement in movement.cpp**

Append to `Game/movement.cpp`:
```cpp
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
```

- [ ] **Step 5: Run all tests**

```bash
./build/tests
```
Expected: all tests pass

- [ ] **Step 6: Commit**

```bash
cd /Users/nickschlosser/RoboticArm-Game
git add Game/movement.h Game/movement.cpp Game/tests/test_movement.cpp
git commit -m "feat: implement arm segment pivot/extend and track base movement"
```

---

## Task 8: Level Definition

**Files:**
- Create: `Game/level.h`
- Create: `Game/level.cpp`

- [ ] **Step 1: Write level.h**

```cpp
// Game/level.h
#pragma once
#include "types.h"

Level make_level_1();
```

- [ ] **Step 2: Write level.cpp**

```cpp
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

    // Platform at row 13, columns 8-16 (object rests here to start)
    for (int x = 8; x <= 16; x++)
        level.tiles[13 * GAME_WIDTH + x] = TileType::SOLID;

    // Arm: two segments — PIVOT then EXTEND
    Arm arm;
    arm.base_x = 1 * BLOCK_SIZE;
    arm.base_y = 9 * BLOCK_SIZE;
    arm.base_angle = 0.0f;
    arm.active_segment = 0;

    Segment seg0{SegmentType::PIVOT,  0.0f, 6 * BLOCK_SIZE};
    Segment seg1{SegmentType::EXTEND, 0.0f, 3 * BLOCK_SIZE};
    arm.segments.push_back(seg0);
    arm.segments.push_back(seg1);

    level.arms.push_back(arm);
    level.active_arm = 0;

    // Object resting on the platform
    level.object = {
        12 * BLOCK_SIZE,
        13 * BLOCK_SIZE - OBJ_H,
        0.0f, 0.0f, false
    };

    // Target zone: on the floor to the right
    level.target_zone = {18 * BLOCK_SIZE, 16 * BLOCK_SIZE, 3 * BLOCK_SIZE, BLOCK_SIZE};

    return level;
}
```

- [ ] **Step 3: Verify project still builds**

```bash
cd /Users/nickschlosser/RoboticArm-Game/Game && cmake --build build --target game 2>&1 | tail -10
```
Note: main.cpp still references old code — this will fail until Task 10. Verify only the `tests` target builds cleanly:
```bash
cmake --build build --target tests && ./build/tests
```
Expected: all tests still pass

- [ ] **Step 4: Commit**

```bash
cd /Users/nickschlosser/RoboticArm-Game
git add Game/level.h Game/level.cpp
git commit -m "feat: define level 1 with platform, arm, and target zone"
```

---

## Task 9: Rendering

**Files:**
- Create: `Game/render.h`
- Create: `Game/render.cpp`

- [ ] **Step 1: Write render.h**

```cpp
// Game/render.h
#pragma once
#include <SDL3/SDL.h>
#include "types.h"

void render_level(SDL_Renderer* renderer, const Level& level, bool won);
void render_arm(SDL_Renderer* renderer, const Arm& arm, bool is_active);
void render_object(SDL_Renderer* renderer, const Object& obj);
void render_ui(SDL_Renderer* renderer, const GameState& gs);
```

- [ ] **Step 2: Write render.cpp**

```cpp
// Game/render.cpp
#include "render.h"
#include "gravity.h"  // compute_fk
#include <vector>

static SDL_FRect to_sdl(const Rect& r) { return {r.x, r.y, r.w, r.h}; }

void render_level(SDL_Renderer* renderer, const Level& level, bool won) {
    // Tiles
    SDL_SetRenderDrawColor(renderer, 100, 100, 110, 255);
    for (int y = 0; y < (int)GAME_HEIGHT; y++) {
        for (int x = 0; x < (int)GAME_WIDTH; x++) {
            if (level.tiles[y * GAME_WIDTH + x] == TileType::SOLID) {
                SDL_FRect r{x * BLOCK_SIZE, y * BLOCK_SIZE, BLOCK_SIZE, BLOCK_SIZE};
                SDL_RenderFillRect(renderer, &r);
            }
        }
    }

    // Target zone
    SDL_SetRenderDrawColor(renderer, won ? 0 : 0, won ? 220 : 180, won ? 100 : 0, 255);
    SDL_FRect tz = to_sdl(level.target_zone);
    SDL_RenderRect(renderer, &tz);

    // Arms
    for (int i = 0; i < (int)level.arms.size(); i++) {
        render_arm(renderer, level.arms[i], i == level.active_arm);
    }
}

void render_arm(SDL_Renderer* renderer, const Arm& arm, bool is_active) {
    std::vector<Vec2> joints = compute_fk(arm);

    for (int i = 0; i < (int)arm.segments.size(); i++) {
        bool selected = is_active && (i == arm.active_segment);

        if (selected)       SDL_SetRenderDrawColor(renderer, 255, 220,   0, 255);
        else if (is_active) SDL_SetRenderDrawColor(renderer, 180, 180, 255, 255);
        else                SDL_SetRenderDrawColor(renderer, 140, 140, 140, 255);

        // Thick line via three parallel offsets
        float x1 = joints[i].x,     y1 = joints[i].y;
        float x2 = joints[i+1].x,   y2 = joints[i+1].y;
        for (int d = -1; d <= 1; d++) {
            SDL_RenderLine(renderer, x1 + d, y1,     x2 + d, y2);
            SDL_RenderLine(renderer, x1,     y1 + d, x2,     y2 + d);
        }
    }

    // Joints as small white squares
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    for (const auto& j : joints) {
        SDL_FRect dot{j.x - 3.0f, j.y - 3.0f, 6.0f, 6.0f};
        SDL_RenderFillRect(renderer, &dot);
    }
}

void render_object(SDL_Renderer* renderer, const Object& obj) {
    SDL_FRect r{obj.x, obj.y, OBJ_W, OBJ_H};
    SDL_SetRenderDrawColor(renderer, 80, 140, 255, 255);
    SDL_RenderFillRect(renderer, &r);
    SDL_SetRenderDrawColor(renderer, 160, 200, 255, 255);
    SDL_RenderRect(renderer, &r);
}

void render_ui(SDL_Renderer* renderer, const GameState& gs) {
    const Arm& arm = gs.level.arms[gs.level.active_arm];
    float ox = 6.0f, oy = 6.0f;
    constexpr float SQ = 8.0f, GAP = 3.0f;

    // One square per segment; yellow if selected
    for (int i = 0; i < (int)arm.segments.size(); i++) {
        bool sel = (arm.active_segment == i);
        SDL_SetRenderDrawColor(renderer,
            sel ? 255 : 80,
            sel ? 220 : 80,
            sel ?   0 : 80,
            255);
        SDL_FRect sq{ox + i * (SQ + GAP), oy, SQ, SQ};
        SDL_RenderFillRect(renderer, &sq);
    }

    // Win: green screen tint
    if (gs.won) {
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, 0, 255, 0, 55);
        SDL_FRect full{0, 0, BLOCK_SIZE * GAME_WIDTH, BLOCK_SIZE * GAME_HEIGHT};
        SDL_RenderFillRect(renderer, &full);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
    }
}
```

- [ ] **Step 3: Commit**

```bash
cd /Users/nickschlosser/RoboticArm-Game
git add Game/render.h Game/render.cpp
git commit -m "feat: implement SDL3 rendering for tiles, arms, object, and UI"
```

---

## Task 10: Wire main.cpp

**Files:**
- Modify: `Game/main.cpp`

- [ ] **Step 1: Rewrite main.cpp**

```cpp
// Game/main.cpp
#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <algorithm>
#include "types.h"
#include "gravity.h"
#include "movement.h"
#include "level.h"
#include "render.h"

#define SDL_WINDOW_WIDTH  (int)(BLOCK_SIZE * GAME_WIDTH)
#define SDL_WINDOW_HEIGHT (int)(BLOCK_SIZE * GAME_HEIGHT)

struct AppState {
    SDL_Window*   window   = nullptr;
    SDL_Renderer* renderer = nullptr;
    GameState gs{};
    Vec2    prev_tip{};
    Uint64  prev_ticks = 0;
};

SDL_AppResult SDL_AppInit(void** appstate, int argc, char* argv[]) {
    SDL_SetAppMetadata("Robotic Arm Game", "0.1", "com.example.robotic-arm");

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    auto* state = new AppState();
    *appstate = state;

    if (!SDL_CreateWindowAndRenderer("Robotic Arm Game",
            SDL_WINDOW_WIDTH, SDL_WINDOW_HEIGHT,
            SDL_WINDOW_RESIZABLE, &state->window, &state->renderer)) {
        SDL_Log("SDL_CreateWindowAndRenderer failed: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    SDL_SetRenderLogicalPresentation(state->renderer,
        SDL_WINDOW_WIDTH, SDL_WINDOW_HEIGHT, SDL_LOGICAL_PRESENTATION_LETTERBOX);

    state->gs.level       = make_level_1();
    state->gs.won         = false;
    state->gs.level_index = 0;
    state->prev_tip       = arm_tip(state->gs.level.arms[0]);
    state->prev_ticks     = SDL_GetTicks();

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event) {
    auto* state = static_cast<AppState*>(appstate);
    GameState& gs = state->gs;

    if (event->type == SDL_EVENT_QUIT) return SDL_APP_SUCCESS;

    if (event->type == SDL_EVENT_KEY_DOWN && !event->key.repeat) {
        Level& level = gs.level;
        Arm&   arm   = level.arms[level.active_arm];
        Object& obj  = level.object;

        switch (event->key.scancode) {
            case SDL_SCANCODE_1: cycle_selection(gs, -1); break;
            case SDL_SCANCODE_2: cycle_selection(gs, +1); break;
            case SDL_SCANCODE_E: {
                if (obj.grabbed) {
                    // Compute tip velocity from prev frame
                    Uint64 now = SDL_GetTicks();
                    float dt = (now - state->prev_ticks) / 1000.0f;
                    Vec2 cur_tip = arm_tip(arm);
                    Vec2 tip_vel{0.0f, 0.0f};
                    if (dt > 0.0f) {
                        tip_vel.x = (cur_tip.x - state->prev_tip.x) / dt;
                        tip_vel.y = (cur_tip.y - state->prev_tip.y) / dt;
                    }
                    release_object(obj, tip_vel);
                } else if (can_grab(obj, arm)) {
                    grab_object(obj);
                }
                break;
            }
            default: break;
        }
    }

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void* appstate) {
    auto* state = static_cast<AppState*>(appstate);
    GameState& gs = state->gs;
    Level& level  = gs.level;

    // Delta time
    Uint64 now = SDL_GetTicks();
    float dt = std::min((now - state->prev_ticks) / 1000.0f, 0.05f);
    state->prev_ticks = now;

    // Track tip for velocity on release
    Arm& active_arm = level.arms[level.active_arm];
    state->prev_tip = arm_tip(active_arm);

    // Held key input
    const bool* keys = SDL_GetKeyboardState(nullptr);
    float delta_angle  = 0.0f, delta_extend = 0.0f, delta_track = 0.0f;
    if (keys[SDL_SCANCODE_LEFT])  delta_angle  = -PIVOT_SPEED  * dt;
    if (keys[SDL_SCANCODE_RIGHT]) delta_angle  =  PIVOT_SPEED  * dt;
    if (keys[SDL_SCANCODE_UP])  { delta_extend =  EXTEND_SPEED * dt; delta_track =  TRACK_SPEED * dt; }
    if (keys[SDL_SCANCODE_DOWN]){ delta_extend = -EXTEND_SPEED * dt; delta_track = -TRACK_SPEED * dt; }

    apply_movement(active_arm, delta_angle, delta_extend, delta_track);

    // Object: snap to tip if grabbed, else simulate
    Object& obj = level.object;
    if (obj.grabbed) {
        Vec2 tip = arm_tip(active_arm);
        obj.x = tip.x - OBJ_W / 2;
        obj.y = tip.y - OBJ_H / 2;
        obj.vx = obj.vy = 0.0f;
    } else {
        update_object(obj, dt);
        resolve_tile_collision(obj, level.tiles, OBJ_W, OBJ_H);
    }

    // Win check
    if (!gs.won) {
        float cx = obj.x + OBJ_W / 2;
        float cy = obj.y + OBJ_H / 2;
        const Rect& tz = level.target_zone;
        if (cx >= tz.x && cx <= tz.x + tz.w && cy >= tz.y && cy <= tz.y + tz.h)
            gs.won = true;
    }

    // Render
    SDL_SetRenderDrawColor(state->renderer, 30, 30, 35, 255);
    SDL_RenderClear(state->renderer);
    render_level(state->renderer, level, gs.won);
    render_object(state->renderer, obj);
    render_ui(state->renderer, gs);
    SDL_RenderPresent(state->renderer);

    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void* appstate, SDL_AppResult result) {
    delete static_cast<AppState*>(appstate);
}
```

- [ ] **Step 2: Build the game**

```bash
cd /Users/nickschlosser/RoboticArm-Game/Game
cmake --build build --target game 2>&1
```
Expected: compiles and links without errors

- [ ] **Step 3: Run the game and verify manually**

```bash
./build/game
```
Expected:
- Window opens (576×432)
- Dark background with gray floor and left wall
- Gray platform in middle area
- Arm visible as lines from left wall
- Blue object sitting on platform
- Green outline target zone on the right
- Press `2` to cycle to segment 1, arrow keys to move
- Press `E` near the object to grab; arm tip snaps object; `E` again releases with velocity
- Object falls under gravity, stops on tiles

- [ ] **Step 4: Run all unit tests one final time**

```bash
cd /Users/nickschlosser/RoboticArm-Game/Game && ./build/tests
```
Expected: all tests pass

- [ ] **Step 5: Commit**

```bash
cd /Users/nickschlosser/RoboticArm-Game
git add Game/main.cpp
git commit -m "feat: wire all systems into SDL3 app — Phase A complete"
```
