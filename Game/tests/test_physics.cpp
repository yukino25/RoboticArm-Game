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

TEST_CASE("update_object: applies gravity and integrates position") {
    Object obj{100.0f, 50.0f, 0.0f, 0.0f, false};

    update_object(obj, 0.5f); // 0.5 seconds

    REQUIRE_THAT(obj.vy, Catch::Matchers::WithinAbs(250.0f, 0.001f));
    REQUIRE_THAT(obj.y,  Catch::Matchers::WithinAbs(175.0f, 0.001f));
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

TEST_CASE("update_object: clamps at left world edge") {
    Object obj{-5.0f, 100.0f, -50.0f, 0.0f, false};
    update_object(obj, 0.0f); // dt=0 so only clamping runs
    REQUIRE_THAT(obj.x,  Catch::Matchers::WithinAbs(0.0f, 0.001f));
    REQUIRE_THAT(obj.vx, Catch::Matchers::WithinAbs(0.0f, 0.001f));
}

TEST_CASE("update_object: clamps at bottom world edge") {
    // GAME_HEIGHT*BLOCK_SIZE = 18*24 = 432. Object bottom = obj.y + OBJ_H (20)
    // Place object so it already exceeds bottom
    float bottom = GAME_HEIGHT * BLOCK_SIZE;
    Object obj{100.0f, bottom - 5.0f, 0.0f, 100.0f, false};
    update_object(obj, 0.0f);
    REQUIRE_THAT(obj.y,  Catch::Matchers::WithinAbs(bottom - OBJ_H, 0.001f));
    REQUIRE_THAT(obj.vy, Catch::Matchers::WithinAbs(0.0f, 0.001f));
}

static void fill_floor(TileType* tiles, int row) {
    for (int x = 0; x < (int)GAME_WIDTH; x++)
        tiles[row * GAME_WIDTH + x] = TileType::SOLID;
}

TEST_CASE("tile collision: object stops on solid floor") {
    TileType tiles[GAME_WIDTH * GAME_HEIGHT];
    std::fill(tiles, tiles + GAME_WIDTH * GAME_HEIGHT, TileType::EMPTY);
    fill_floor(tiles, 5); // floor top at y = 5*24 = 120

    // Object overlaps floor
    Object obj{48.0f, 5 * BLOCK_SIZE - 10.0f, 0.0f, 200.0f, false};

    resolve_tile_collision(obj, tiles, OBJ_W, OBJ_H);

    REQUIRE_THAT(obj.y,  Catch::Matchers::WithinAbs(5 * BLOCK_SIZE - OBJ_H, 0.5f));
    REQUIRE_THAT(obj.vy, Catch::Matchers::WithinAbs(0.0f, 0.001f));
}

TEST_CASE("tile collision: object stops against left wall") {
    TileType tiles[GAME_WIDTH * GAME_HEIGHT];
    std::fill(tiles, tiles + GAME_WIDTH * GAME_HEIGHT, TileType::EMPTY);
    for (int y = 0; y < (int)GAME_HEIGHT; y++)
        tiles[y * GAME_WIDTH + 3] = TileType::SOLID;

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

static Arm make_horizontal_arm(float base_x, float base_y, float length) {
    Arm arm;
    arm.base_x = base_x; arm.base_y = base_y; arm.base_angle = 0.0f;
    arm.active_segment = 0;
    arm.segments.push_back({SegmentType::PIVOT, 0.0f, length});
    return arm;
}

TEST_CASE("can_grab: true when object center within GRAB_RADIUS of tip") {
    // Tip at (100, 0). Object center at (95, 0) → distance 5 < 12
    Arm arm = make_horizontal_arm(0.0f, 0.0f, 100.0f);
    Object obj{95.0f - OBJ_W / 2, -OBJ_H / 2, 0.0f, 0.0f, false};
    REQUIRE(can_grab(obj, arm) == true);
}

TEST_CASE("can_grab: false when object center beyond GRAB_RADIUS") {
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
