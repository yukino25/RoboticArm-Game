# Robotic Arm Game — Design Spec
Date: 2026-04-02

## Overview

A 2D side-scrolling puzzle game where the player controls one or two robotic arms to grab an object and move it to a target location. The arm is made of segments that can pivot or extend. Gravity affects the object. Different levels vary the number of segments, arm count, and base mobility.

---

## 1. Data Model

### Segment
The atomic unit of an arm.

```cpp
enum SegmentType { PIVOT, EXTEND, BOTH };

struct Segment {
    SegmentType type;
    float angle;   // radians, relative to parent segment
    float length;  // pixels
};
```

Positions are never stored on segments — they are computed each frame by walking the chain (forward kinematics).

### Arm

```cpp
struct Track {
    float min, max;  // world-space range
    bool horizontal; // true = slides on X axis, false = Y axis
};

struct Arm {
    std::vector<Segment> segments;
    float base_x, base_y;       // world position of the arm root
    float base_angle;           // rotation offset applied before the chain
    std::optional<Track> track; // present only in track-based levels
    int active_segment;         // index of currently selected segment
};
```

### Object
The item the player must move to the target.

```cpp
struct Object {
    float x, y;
    float vx, vy;
    bool grabbed;
};
```

### Level

```cpp
enum TileType { EMPTY, SOLID };

struct Level {
    TileType tiles[GAME_WIDTH * GAME_HEIGHT]; // 24x18 grid
    std::vector<Arm> arms;                    // 1 or 2 arms
    Object object;
    SDL_FRect target_zone; // win condition: object center must land inside this
    int active_arm;        // index of arm currently receiving input
};
```

### GameState
Wraps the active level and tracks overall session state (current level index, win flag).

---

## 2. Game Loop

SDL3's `SDL_AppIterate` callback runs once per frame. Order of operations:

1. **Process input** — translate held keys into actions for the current frame
2. **Update arm** — apply pivot/extend delta to the selected segment; recompute FK chain positions
3. **Update object** — apply gravity, integrate velocity, resolve tile collisions; if grabbed, snap position to arm tip
4. **Check win** — if object center is inside `target_zone`, mark level complete

---

## 3. Input

| Key | Action |
|-----|--------|
| `1` | Cycle selection backward through segments; wraps from arm 0 segment 0 to arm 1 last segment (if arm 1 exists) |
| `2` | Cycle selection forward through segments; wraps from arm 1 last segment back to arm 0 segment 0 |
| `←` / `→` | Rotate selected segment counter-clockwise / clockwise (PIVOT or BOTH types only) |
| `↑` / `↓` | Extend / retract selected segment (EXTEND or BOTH types only); move base along track if base is selected (segment_index == -1) |
| `E` | Toggle grab — attach object if arm tip is within grab radius (~12px) of object center; detach otherwise |

**Selection model:** A single `(arm_index, segment_index)` cursor. `segment_index == -1` means the arm's base is selected (only relevant for track-based arms). Pressing `1`/`2` moves the cursor: base (-1) → segment 0 → segment 1 → ... → last segment of arm 0, then into arm 1 if present. When only one arm exists, it cycles within that arm only.

All key-held movement is smooth (continuous delta per frame, scaled by `dt`).

---

## 4. Physics

### Arm (kinematic)
The arm has no physics simulation. Positions are computed purely from the segment chain:

```
joint[0] = base_pos (rotated by base_angle)
for i in 0..n:
    cumulative_angle = base_angle + sum(segment[0..i].angle)
    joint[i+1] = joint[i] + (cos(cumulative_angle) * segment[i].length,
                              sin(cumulative_angle) * segment[i].length)
arm_tip = joint[n+1]
```

### Constants
- `GRAVITY` = 500 px/s²
- `PIVOT_SPEED` = 2.0 rad/s
- `EXTEND_SPEED` = 80 px/s
- `TRACK_SPEED` = 100 px/s
- `GRAB_RADIUS` = 12 px
- `MIN_SEGMENT_LENGTH` = 10 px (segments cannot retract below this)

### Object physics
Each frame:
```
vel.y += GRAVITY * dt
pos += vel * dt
```

- **On grab:** `pos = arm_tip`, `vel = {0, 0}`, gravity suspended
- **On release:** object inherits the arm tip's velocity from the previous frame (enables momentum-based throwing and gravity drops)
- **Tile collision:** AABB sweep against solid tiles; resolve by separating axis (stop on contact, zero velocity on the colliding axis)

### Grab detection
Grab triggers when `distance(object.pos, arm_tip) <= GRAB_RADIUS` (approximately 12px).

### Segment collision (Phase A → B)
- **Phase A (initial):** Segments are fully kinematic. They do not collide with tiles. Players must manually avoid walls.
- **Phase B (later):** Before applying a movement delta, check whether the new segment endpoints intersect any solid tile. If so, clamp the delta to the last valid position and do not move further.

---

## 5. Rendering

All drawing uses SDL3 primitives (rectangles and lines). No sprites in the initial implementation.

**Draw order (back to front):**
1. **Tile grid** — solid tiles as filled gray rectangles
2. **Track** (if present) — thin line behind the base joint showing the slide range
3. **Target zone** — outlined rectangle in green
4. **Arm segments** — thick line from joint to joint for each segment; selected segment drawn in a highlight color
5. **Joints** — small filled circle at each joint position
6. **Object** — filled colored rectangle
7. **UI overlay** — selection indicator in a screen corner (e.g., "Arm 1 | Seg 3")
8. **Win flash** — brief color tint or target zone flash when the object lands in the target

**Coordinate system:** FK joint positions are in world-space pixels. The screen is fixed (24×18 tiles × 24px = 576×432px logical resolution). No camera transform is needed.

---

## 6. Level Structure

Levels are defined by:
- The tile grid (walls and platforms as SOLID cells)
- Number and configuration of arms (segment count, types, base position, track if any)
- Object starting position
- Target zone position and size

Early levels: one arm, few segments, simple obstacle. Later levels: two arms, mixed segment types, base on track, gravity-dependent drops required.

---

## 7. Phased Implementation

| Phase | Scope |
|-------|-------|
| A | FK chain, single arm, pivot + extend, object with gravity, grab/release, tile collision for object, one hardcoded level |
| B | Segment collision against tiles (clamp delta), win condition, level loading |
| C | Two-arm support, track-based base movement, additional levels |
