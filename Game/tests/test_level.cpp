#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <fstream>
#include <cstdio>
#include "../types.h"
#include "../level.h"

// Write a temp file and return its path
static std::string write_temp(const std::string& name, const std::string& contents) {
    std::string path = "/tmp/" + name;
    std::ofstream f(path);
    f << contents;
    return path;
}

static const std::string MINIMAL_LEVEL =
"tiles:\n"
"########################\n"
"#......................#\n"
"#......................#\n"
"#......................#\n"
"#......................#\n"
"#......................#\n"
"#......................#\n"
"#......................#\n"
"#......................#\n"
"#......................#\n"
"#......................#\n"
"#......................#\n"
"#......................#\n"
"#......................#\n"
"#......................#\n"
"#......................#\n"
"#......................#\n"
"########################\n"
"\n"
"arm:\n"
"base 24 216\n"
"angle 0\n"
"segment pivot 144\n"
"segment both 72\n"
"\n"
"object:\n"
"pos 288 288\n"
"\n"
"target:\n"
"pos 432 384 72 24\n";

TEST_CASE("load_level: returns nullopt for missing file") {
    auto result = load_level("/tmp/does_not_exist_xyz.level");
    REQUIRE(!result.has_value());
}

TEST_CASE("load_level: parses tile grid correctly") {
    auto path = write_temp("test_tiles.level", MINIMAL_LEVEL);
    auto result = load_level(path);
    REQUIRE(result.has_value());

    // Top-left corner should be SOLID (#)
    REQUIRE(result->tiles[0 * GAME_WIDTH + 0] == TileType::SOLID);
    // Interior should be EMPTY (.)
    REQUIRE(result->tiles[1 * GAME_WIDTH + 1] == TileType::EMPTY);
    // Bottom-right corner should be SOLID
    REQUIRE(result->tiles[17 * GAME_WIDTH + 23] == TileType::SOLID);
}

TEST_CASE("load_level: parses arm correctly") {
    auto path = write_temp("test_arm.level", MINIMAL_LEVEL);
    auto result = load_level(path);
    REQUIRE(result.has_value());

    REQUIRE(result->arms.size() == 1);
    const Arm& arm = result->arms[0];
    REQUIRE_THAT(arm.base_x, Catch::Matchers::WithinAbs(24.0f,  0.001f));
    REQUIRE_THAT(arm.base_y, Catch::Matchers::WithinAbs(216.0f, 0.001f));
    REQUIRE_THAT(arm.base_angle, Catch::Matchers::WithinAbs(0.0f, 0.001f));
    REQUIRE(arm.segments.size() == 2);
    REQUIRE(arm.segments[0].type == SegmentType::PIVOT);
    REQUIRE_THAT(arm.segments[0].length, Catch::Matchers::WithinAbs(144.0f, 0.001f));
    REQUIRE(arm.segments[1].type == SegmentType::BOTH);
    REQUIRE_THAT(arm.segments[1].length, Catch::Matchers::WithinAbs(72.0f, 0.001f));
}

TEST_CASE("load_level: parses object and target correctly") {
    auto path = write_temp("test_obj.level", MINIMAL_LEVEL);
    auto result = load_level(path);
    REQUIRE(result.has_value());

    REQUIRE_THAT(result->object.x, Catch::Matchers::WithinAbs(288.0f, 0.001f));
    REQUIRE_THAT(result->object.y, Catch::Matchers::WithinAbs(288.0f, 0.001f));
    REQUIRE(result->object.grabbed == false);
    REQUIRE_THAT(result->object.vx, Catch::Matchers::WithinAbs(0.0f, 0.001f));
    REQUIRE_THAT(result->object.vy, Catch::Matchers::WithinAbs(0.0f, 0.001f));

    REQUIRE_THAT(result->target_zone.x, Catch::Matchers::WithinAbs(432.0f, 0.001f));
    REQUIRE_THAT(result->target_zone.y, Catch::Matchers::WithinAbs(384.0f, 0.001f));
    REQUIRE_THAT(result->target_zone.w, Catch::Matchers::WithinAbs(72.0f,  0.001f));
    REQUIRE_THAT(result->target_zone.h, Catch::Matchers::WithinAbs(24.0f,  0.001f));
}

TEST_CASE("load_level: returns nullopt for wrong tile row count") {
    // Only 17 tile rows instead of 18
    std::string bad =
        "tiles:\n"
        "########################\n"
        "########################\n"
        "########################\n"
        "########################\n"
        "########################\n"
        "########################\n"
        "########################\n"
        "########################\n"
        "########################\n"
        "########################\n"
        "########################\n"
        "########################\n"
        "########################\n"
        "########################\n"
        "########################\n"
        "########################\n"
        "########################\n"
        "\n"
        "arm:\nbase 0 0\nangle 0\nsegment pivot 50\n"
        "\nobject:\npos 0 0\n\ntarget:\npos 0 0 10 10\n";
    auto path = write_temp("bad_tiles.level", bad);
    auto result = load_level(path);
    REQUIRE(!result.has_value());
}
