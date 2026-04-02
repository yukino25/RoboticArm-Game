#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
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
    arm.active_segment = has_track ? -1 : 0;
    if (has_track) arm.track = Track{0.0f, 100.0f, true};
    for (int i = 0; i < num_segments; i++) {
        Segment s{SegmentType::PIVOT, 0.0f, 50.0f};
        arm.segments.push_back(s);
    }
    level.arms.push_back(arm);
    return gs;
}

TEST_CASE("cycle_selection: forward moves base to seg0") {
    GameState gs = make_gs_one_arm(2, /*has_track=*/true);
    REQUIRE(gs.level.arms[0].active_segment == -1);

    cycle_selection(gs, +1);
    REQUIRE(gs.level.arms[0].active_segment == 0);
}

TEST_CASE("cycle_selection: forward advances through segments") {
    GameState gs = make_gs_one_arm(2, true);
    gs.level.arms[0].active_segment = 0;

    cycle_selection(gs, +1);
    REQUIRE(gs.level.arms[0].active_segment == 1);
}

TEST_CASE("cycle_selection: forward wraps last segment back to base (track arm)") {
    GameState gs = make_gs_one_arm(2, true);
    gs.level.arms[0].active_segment = 1; // at last segment

    cycle_selection(gs, +1);
    REQUIRE(gs.level.arms[0].active_segment == -1);
}

TEST_CASE("cycle_selection: forward wraps last segment back to seg0 (no track)") {
    GameState gs = make_gs_one_arm(2, false);
    gs.level.arms[0].active_segment = 1;

    cycle_selection(gs, +1);
    REQUIRE(gs.level.arms[0].active_segment == 0);
}

TEST_CASE("cycle_selection: backward from seg0 goes to base (track arm)") {
    GameState gs = make_gs_one_arm(2, true);
    gs.level.arms[0].active_segment = 0;

    cycle_selection(gs, -1);
    REQUIRE(gs.level.arms[0].active_segment == -1);
}

TEST_CASE("cycle_selection: crosses from arm0 last seg to arm1") {
    GameState gs = make_gs_one_arm(2, false);
    // Add second arm
    Arm arm2;
    arm2.base_x = 0; arm2.base_y = 0; arm2.base_angle = 0;
    arm2.active_segment = 0;
    arm2.segments.push_back({SegmentType::PIVOT, 0.0f, 50.0f});
    gs.level.arms.push_back(arm2);

    gs.level.arms[0].active_segment = 1; // last seg of arm0

    cycle_selection(gs, +1);

    REQUIRE(gs.level.active_arm == 1);
    REQUIRE(gs.level.arms[1].active_segment == 0);
}

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
    apply_movement(arm, 0.0f, -100.0f, 0.0f);
    REQUIRE_THAT(arm.segments[0].length, Catch::Matchers::WithinAbs(MIN_SEG_LEN, 0.001f));
}

TEST_CASE("apply_movement: BOTH responds to angle and length") {
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
    apply_movement(arm, 0.0f, 0.0f, 50.0f);
    REQUIRE_THAT(arm.base_x, Catch::Matchers::WithinAbs(200.0f, 0.001f));
}
