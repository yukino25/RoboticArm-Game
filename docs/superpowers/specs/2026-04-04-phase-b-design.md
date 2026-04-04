# Phase B Design Spec
Date: 2026-04-04

## Overview

Phase B adds two features to the robotic arm puzzle game:
1. **Segment collision** — arm segments are blocked by solid tiles and by other arms' segments
2. **Level file loading** — levels are loaded from plain-text `.level` files instead of hardcoded C++ functions

---

## 1. Segment Collision Against Tiles

### Problem
In Phase A, arm segments pass freely through solid tiles. Phase B stops this.

### Algorithm
Before committing a movement delta, binary-search between `lo=0` (no movement) and `hi=requested delta` to find the largest delta that leaves all segment joints clear of solid tiles.

```
lo = 0
hi = requested_delta
for 8 iterations:
    mid = (lo + hi) / 2
    tentatively apply mid to the segment, recompute FK
    if any joint endpoint overlaps a solid tile:
        hi = mid
    else:
        lo = mid
apply lo to segment
```

**Collision test:** Each joint position (endpoint of a segment) is tested against the tile grid. A joint at `(x, y)` overlaps a solid tile if the tile at `(floor(x/BLOCK_SIZE), floor(y/BLOCK_SIZE))` is `SOLID`. Only joint endpoints are tested — not the full line between joints. This is sufficient for the segment sizes in this game.

**Implementation:** New function `clamp_segment_delta` in `movement.cpp`, called from `apply_movement` before any delta is applied. Returns the safe delta value.

### Constants
- `SEGMENT_COLLISION_ITERATIONS = 8` — binary search iterations per frame (added to `types.h`)

---

## 2. Arm-to-Arm Segment Collision

### Problem
Arm segments can pass through each other's joints. Phase B stops this.

### Algorithm
After the tile-clamping pass, a second binary-search runs against all other arms' joint positions:

```
lo = 0
hi = tile_clamped_delta
for 8 iterations:
    mid = (lo + hi) / 2
    tentatively apply mid to the moving arm, recompute FK
    for each joint of moving arm:
        for each joint of every other arm:
            if distance(jointA, jointB) < ARM_RADIUS:
                hi = mid
                goto next_iteration
    lo = mid
apply lo to segment
```

**Collision model:** Joint-to-joint distance only. No line-segment intersection math.

**Key constraint:** Only the *moving* arm is clamped. The stationary arm does not move. If the player switches to the other arm and moves it, that arm is then clamped instead.

### Constants
- `ARM_RADIUS = 8.0f` px — minimum allowed distance between any two joints of different arms (added to `types.h`)

### Implementation
Both passes (tile + arm-to-arm) are folded into a single `clamp_segment_delta` function signature:

```cpp
// Returns the largest safe delta in [-|delta|, |delta|] that keeps all
// joints of `arm` clear of solid tiles and away from other arms' joints.
float clamp_segment_delta(
    Arm arm,                         // copy — mutated during search
    int seg_idx,                     // which segment is being moved
    bool is_angle,                   // true=angle delta, false=length delta
    float requested_delta,
    const TileType* tiles,
    const std::vector<Arm>& all_arms,
    int moving_arm_idx               // excluded from arm-to-arm check
);
```

`apply_movement` calls `clamp_segment_delta` twice — once for angle, once for length — replacing the direct segment mutation.

---

## 3. Level File Format

### File Location
`Game/levels/` directory. Files use the `.level` extension. `main.cpp` passes the path as a string, e.g. `"levels/level1.level"`.

### Format

```
# Level 1
tiles:
##########################
#.......................#
#.......................#
#.......................#
#.......................#
#.......................#
#.......................#
#.......................#
#.......................#
#.......................#
#.......................#
#.......................#
#.......................#
#........#########......#
#.......................#
#.......................#
#..................###...#
##########################

arm:
base 24 216
angle 0
segment pivot 144
segment both 72

object:
pos 288 288

target:
pos 432 384 72 24
```

### Rules
- The tile grid is always exactly 24 columns × 18 rows. `#` = SOLID, `.` = EMPTY.
- `arm:` block repeats for each arm in the level (order = arm index).
  - `base <x> <y>` — world-space pixel position of the arm root
  - `angle <radians>` — base rotation
  - `segment <type> <length>` — one line per segment; type is `pivot`, `extend`, or `both`
- `object:` section: `pos <x> <y>` — object top-left in pixels, spawns with zero velocity
- `target:` section: `pos <x> <y> <w> <h>` — target zone in pixels
- Lines beginning with `#` that appear before the first section header are comments. `#` inside the tile grid is a tile character, not a comment.
- Blank lines are ignored.

### Parsing
New function in `level.cpp`:

```cpp
std::optional<Level> load_level(const std::string& path);
```

- Opens the file with `std::ifstream`. Returns `std::nullopt` if the file cannot be opened.
- Parses line by line, tracking the current section (`tiles`, `arm`, `object`, `target`).
- Returns `std::nullopt` on any parse error (wrong column count, unknown segment type, missing required section).
- `make_level_1()` is removed. `Game/levels/level1.level` encodes the equivalent layout.

### main.cpp change
`SDL_AppInit` loads the level:
```cpp
auto lvl = load_level("levels/level1.level");
if (!lvl) { SDL_Log("Failed to load level"); return SDL_APP_FAILURE; }
state->gs.level = *lvl;
```

---

## 4. File Changes Summary

| File | Change |
|------|--------|
| `Game/types.h` | Add `SEGMENT_COLLISION_ITERATIONS`, `ARM_RADIUS` constants |
| `Game/movement.h` | Declare `clamp_segment_delta`; update `apply_movement` signature to take `tiles` + `all_arms` + `moving_arm_idx` |
| `Game/movement.cpp` | Implement `clamp_segment_delta`; update `apply_movement` to call it |
| `Game/level.h` | Declare `load_level`; remove `make_level_1` |
| `Game/level.cpp` | Implement `load_level`; remove `make_level_1` |
| `Game/main.cpp` | Use `load_level` instead of `make_level_1`; pass `level.tiles` and `level.arms` to `apply_movement` |
| `Game/levels/level1.level` | New file — Level 1 in text format |
| `Game/tests/test_movement.cpp` | Add tests for `clamp_segment_delta` |
| `Game/tests/test_level.cpp` | New file — tests for `load_level` |
| `Game/CMakeLists.txt` | Add `test_level.cpp` to `tests` target |

---

## 5. apply_movement Updated Signature

```cpp
void apply_movement(
    Arm& arm,
    float delta_angle,
    float delta_extend,
    float delta_track,
    const TileType* tiles,
    const std::vector<Arm>& all_arms,
    int moving_arm_idx
);
```

The three new parameters are passed down to `clamp_segment_delta`. Track-base movement uses only tile collision (no arm-to-arm check on the base position).
