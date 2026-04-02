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
