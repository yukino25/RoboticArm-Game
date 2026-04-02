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
