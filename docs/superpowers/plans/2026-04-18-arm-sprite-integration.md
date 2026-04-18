# Arm Sprite Integration Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Replace colored-line arm rendering with the pixel-art arm sprites in `Game/assets/arm/`.

**Architecture:** Add arm texture fields to `Textures`, load them in `load_textures`, rewrite `render_arm` to use `SDL_RenderTextureRotated` per segment type (PIVOT → `arm_segment`, EXTEND/BOTH → `arm_outer` + `arm_inner` overlap), and attach the suction grabber at the tip. Update `level1.level` so all segment lengths are valid standard sizes.

**Tech Stack:** C++17, SDL3, SDL3_image, `SDL_RenderTextureRotated`.

---

## File Map

| File | Change |
|------|--------|
| `Game/render.h` | Add `ArmTexSet` struct and arm texture fields to `Textures` |
| `Game/render.cpp` | Update `load_textures`, `free_textures`, rewrite `render_arm` |
| `Game/levels/level1.level` | Update segment lengths to valid standard sizes |

---

## Background: Sprite Sizes

| Index | Variant | Segment length | `arm_segment` W×H | `arm_inner` W×H | `arm_outer` W×H |
|-------|---------|---------------|-------------------|-----------------|-----------------|
| 0     | `_sm`   | 32 px         | 32 × 16           | 48 × 12         | 48 × 20         |
| 1     | `_md`   | 48 px         | 48 × 16           | 64 × 12         | 64 × 20         |
| 2     | `_lg`   | 64 px         | 64 × 16           | 80 × 12         | 80 × 20         |
| 3     | `_xl`   | 80 px         | 80 × 16           | 96 × 12         | 96 × 20         |

`arm_grab_suction` is always 24 × 20. Its pivot circle centre is at sprite-local (12, 4).

---

## Background: Render Math

### PIVOT segment (joint[i] → joint[i+1])
```
mid     = (joint[i] + joint[i+1]) * 0.5
angle   = atan2(dy, dx) * 180/π        // degrees, SDL clockwise
dstrect = { mid.x - seg.length/2,  mid.y - 8,  seg.length, 16 }
center  = { seg.length/2, 8 }          // rotates around mid ✓
```

### EXTEND/BOTH — arm_outer (draw first, behind)
```
dstrect = { j0.x, j0.y - 10,  outer_w, 20 }
center  = { 0, 10 }                    // rotates around j0 ✓
angle   = segment_angle_deg
```

### EXTEND/BOTH — arm_inner (draw second, on top)
```
dstrect = { j1.x, j1.y - 6,  inner_w, 12 }
center  = { 0, 6 }                     // rotates around j1 ✓
angle   = segment_angle_deg + 180      // points inner back toward j0
```
At short extensions the inner's far end disappears behind the wall at j0. At long extensions the inner emerges visibly from the outer's open end. Draw outer on top of inner so the sleeve hides the rod where they overlap.

### Suction grabber (tip of last joint)
```
dstrect = { tip.x - 12, tip.y - 4,  24, 20 }
center  = { 12, 4 }                    // rotates around tip ✓
angle   = last_segment_angle_deg - 90  // rubber pad faces outward
```

---

## Task 1 — Update `Textures` struct in `render.h`

**Files:**
- Modify: `Game/render.h`

- [ ] **Step 1: Replace the current `Textures` struct**

Open `Game/render.h`. Replace the entire `Textures` struct with:

```cpp
struct ArmTexSet {
    SDL_Texture* seg_active   = nullptr;   // arm_segment_{size}_active
    SDL_Texture* seg_inactive = nullptr;   // arm_segment_{size}_inactive
    SDL_Texture* outer_active = nullptr;   // arm_outer_{size}_active
    SDL_Texture* outer_inactive = nullptr; // arm_outer_{size}_inactive
    SDL_Texture* inner        = nullptr;   // arm_inner_{size}
};

struct Textures {
    SDL_Texture* tile    = nullptr;
    SDL_Texture* bg      = nullptr;
    SDL_Texture* object  = nullptr;
    ArmTexSet    arm[4]{};                 // [0]=sm [1]=md [2]=lg [3]=xl
    SDL_Texture* grab_suction = nullptr;
};
```

- [ ] **Step 2: Commit**

```bash
git add Game/render.h
git commit -m "feat: add arm texture fields to Textures struct"
```

---

## Task 2 — Load arm textures in `render.cpp`

**Files:**
- Modify: `Game/render.cpp`

- [ ] **Step 1: Replace `load_textures` and `free_textures`**

In `Game/render.cpp`, replace the two existing functions with:

```cpp
bool load_textures(SDL_Renderer* renderer, Textures& out) {
    out.tile   = load_tex(renderer, "assets/tiles/IndustrialTile_26.png");
    out.bg     = load_tex(renderer, "assets/background/Background.png");
    out.object = load_tex(renderer, "assets/objects/Box1.png");

    const char* sizes[4] = { "sm", "md", "lg", "xl" };
    for (int i = 0; i < 4; i++) {
        char buf[128];
        SDL_snprintf(buf, sizeof(buf), "assets/arm/arm_segment_%s_active.png",   sizes[i]);
        out.arm[i].seg_active   = load_tex(renderer, buf);
        SDL_snprintf(buf, sizeof(buf), "assets/arm/arm_segment_%s_inactive.png", sizes[i]);
        out.arm[i].seg_inactive = load_tex(renderer, buf);
        SDL_snprintf(buf, sizeof(buf), "assets/arm/arm_outer_%s_active.png",     sizes[i]);
        out.arm[i].outer_active   = load_tex(renderer, buf);
        SDL_snprintf(buf, sizeof(buf), "assets/arm/arm_outer_%s_inactive.png",   sizes[i]);
        out.arm[i].outer_inactive = load_tex(renderer, buf);
        SDL_snprintf(buf, sizeof(buf), "assets/arm/arm_inner_%s.png",            sizes[i]);
        out.arm[i].inner = load_tex(renderer, buf);
    }
    out.grab_suction = load_tex(renderer, "assets/arm/arm_grab_suction.png");

    return out.tile && out.bg && out.object;
}

void free_textures(Textures& t) {
    SDL_DestroyTexture(t.tile);
    SDL_DestroyTexture(t.bg);
    SDL_DestroyTexture(t.object);
    for (int i = 0; i < 4; i++) {
        SDL_DestroyTexture(t.arm[i].seg_active);
        SDL_DestroyTexture(t.arm[i].seg_inactive);
        SDL_DestroyTexture(t.arm[i].outer_active);
        SDL_DestroyTexture(t.arm[i].outer_inactive);
        SDL_DestroyTexture(t.arm[i].inner);
    }
    SDL_DestroyTexture(t.grab_suction);
    t = {};
}
```

- [ ] **Step 2: Build to confirm it compiles**

```bash
cmake --build Game/build --parallel 2>&1 | tail -20
```
Expected: no errors (warnings about unused variables are OK).

- [ ] **Step 3: Commit**

```bash
git add Game/render.cpp
git commit -m "feat: load and free arm sprite textures"
```

---

## Task 3 — Add `arm_size_index` helper and update `render_arm` signature

**Files:**
- Modify: `Game/render.h`, `Game/render.cpp`

The helper maps `segment.length` to an `arm[]` index. Valid lengths are 32/48/64/80 (indices 0-3).

- [ ] **Step 1: Update `render_arm` signature in `render.h`**

Change the declaration from:
```cpp
void render_arm(SDL_Renderer* renderer, const Arm& arm, bool is_active);
```
to:
```cpp
void render_arm(SDL_Renderer* renderer, const Textures& tex, const Arm& arm, bool is_active);
```

- [ ] **Step 2: Add helper and stub in `render.cpp`**

Above the existing `render_arm` definition, add:

```cpp
// Returns arm[] index for the given segment length. Returns -1 for invalid lengths.
static int arm_size_index(float len) {
    if (len == 32.0f) return 0;
    if (len == 48.0f) return 1;
    if (len == 64.0f) return 2;
    if (len == 80.0f) return 3;
    return -1;
}
```

- [ ] **Step 3: Update the call site in `main.cpp`**

In `SDL_AppIterate`, find the render_level call. `render_level` calls `render_arm` internally, so update `render_level` signature in the same pass (next task). For now just note the call chain: `main.cpp → render_level → render_arm`.

- [ ] **Step 4: Update `render_level` to forward `tex` to `render_arm`**

In `render.cpp`, in `render_level`, the existing call is:
```cpp
render_arm(renderer, level.arms[i], i == level.active_arm);
```
Change it to:
```cpp
render_arm(renderer, tex, level.arms[i], i == level.active_arm);
```

- [ ] **Step 5: Build**

```bash
cmake --build Game/build --parallel 2>&1 | tail -20
```
Expected: no errors.

- [ ] **Step 6: Commit**

```bash
git add Game/render.h Game/render.cpp
git commit -m "feat: add arm_size_index helper, thread tex into render_arm"
```

---

## Task 4 — Rewrite `render_arm` — PIVOT segments

**Files:**
- Modify: `Game/render.cpp`

- [ ] **Step 1: Add `<cmath>` include at the top of render.cpp if not present**

Check the top of `Game/render.cpp`. If `<cmath>` is not included, add it:
```cpp
#include <cmath>
```

- [ ] **Step 2: Replace the body of `render_arm` with the new implementation (PIVOT only first)**

Replace the entire `render_arm` function body with:

```cpp
void render_arm(SDL_Renderer* renderer, const Textures& tex,
                const Arm& arm, bool is_active) {
    std::vector<Vec2> joints = compute_fk(arm);

    for (int i = 0; i < (int)arm.segments.size(); i++) {
        const Segment& seg = arm.segments[i];
        const Vec2& j0 = joints[i];
        const Vec2& j1 = joints[i + 1];

        float dx = j1.x - j0.x;
        float dy = j1.y - j0.y;
        double angle_deg = std::atan2((double)dy, (double)dx) * 180.0 / M_PI;

        bool selected = is_active && (i == arm.active_segment);
        int si = arm_size_index(seg.length);

        if (seg.type == SegmentType::PIVOT) {
            // ── PIVOT: render arm_segment centred between j0 and j1 ──────
            if (si < 0) {
                // Fallback: solid coloured line for invalid length
                SDL_SetRenderDrawColor(renderer, 200, 50, 50, 255);
                SDL_RenderLine(renderer, j0.x, j0.y, j1.x, j1.y);
                continue;
            }
            SDL_Texture* t = selected
                ? tex.arm[si].seg_active
                : tex.arm[si].seg_inactive;
            if (!t) continue;

            float sw = seg.length;
            float sh = 16.0f;
            float mx = (j0.x + j1.x) * 0.5f;
            float my = (j0.y + j1.y) * 0.5f;
            SDL_FRect dst = { mx - sw * 0.5f, my - sh * 0.5f, sw, sh };
            SDL_FPoint cen = { sw * 0.5f, sh * 0.5f };
            SDL_RenderTextureRotated(renderer, t, nullptr, &dst,
                                     angle_deg, &cen, SDL_FLIP_NONE);
        }
        // EXTEND / BOTH handled in next task
    }

    // Suction grabber at tip (placeholder until Task 5)
    const Vec2& tip = joints.back();
    SDL_SetRenderDrawColor(renderer, 255, 200, 0, 255);
    SDL_FRect dot{ tip.x - 4.0f, tip.y - 4.0f, 8.0f, 8.0f };
    SDL_RenderFillRect(renderer, &dot);
}
```

- [ ] **Step 3: Build and run — verify PIVOT segments render as sprites**

```bash
cmake --build Game/build --parallel && cd Game/build && ./game
```
Expected: PIVOT arm segment shows the metallic sprite, rotates correctly, LED lights on selected segment.

- [ ] **Step 4: Commit**

```bash
git add Game/render.cpp
git commit -m "feat: render PIVOT arm segments with sprites"
```

---

## Task 5 — Extend `render_arm` — EXTEND/BOTH segments + suction grabber

**Files:**
- Modify: `Game/render.cpp`

- [ ] **Step 1: Add EXTEND/BOTH branch inside the segment loop**

In `render_arm`, after the `PIVOT` branch (inside the `for` loop), add:

```cpp
        else {
            // ── EXTEND / BOTH: arm_inner (behind) then arm_outer (on top) ──
            // Use _sm (index 0) outer/inner for any telescoping segment.
            // The inner slides in/out; overlap hides the rod inside the sleeve.
            int esi = (si >= 0) ? si : 0;  // prefer matched size, fall back to sm

            float ow = 48.0f + esi * 16.0f;   // outer widths: 48,64,80,96
            float iw = 48.0f + esi * 16.0f;   // inner widths: same
            float oh = 20.0f;
            float ih = 12.0f;

            // Draw inner rod first (behind outer sleeve)
            SDL_Texture* ti = tex.arm[esi].inner;
            if (ti) {
                SDL_FRect dst_i = { j1.x, j1.y - ih * 0.5f, iw, ih };
                SDL_FPoint cen_i = { 0.0f, ih * 0.5f };
                SDL_RenderTextureRotated(renderer, ti, nullptr, &dst_i,
                                         angle_deg + 180.0, &cen_i, SDL_FLIP_NONE);
            }

            // Draw outer sleeve on top
            SDL_Texture* to = selected
                ? tex.arm[esi].outer_active
                : tex.arm[esi].outer_inactive;
            if (to) {
                SDL_FRect dst_o = { j0.x, j0.y - oh * 0.5f, ow, oh };
                SDL_FPoint cen_o = { 0.0f, oh * 0.5f };
                SDL_RenderTextureRotated(renderer, to, nullptr, &dst_o,
                                         angle_deg, &cen_o, SDL_FLIP_NONE);
            }
        }
```

- [ ] **Step 2: Replace the tip placeholder with the suction grabber**

After the segment loop closes, replace the yellow dot placeholder with:

```cpp
    // ── Suction grabber at arm tip ─────────────────────────────────────────
    if (tex.grab_suction) {
        const Vec2& tip = joints.back();
        // Angle of the final segment, used to orient the grabber outward.
        float last_dx = joints.back().x - joints[joints.size()-2].x;
        float last_dy = joints.back().y - joints[joints.size()-2].y;
        double grab_angle = std::atan2((double)last_dy, (double)last_dx)
                            * 180.0 / M_PI - 90.0;
        // Pivot circle of suction sprite is at sprite-local (12, 4).
        SDL_FRect dst_g = { tip.x - 12.0f, tip.y - 4.0f, 24.0f, 20.0f };
        SDL_FPoint cen_g = { 12.0f, 4.0f };
        SDL_RenderTextureRotated(renderer, tex.grab_suction, nullptr, &dst_g,
                                 grab_angle, &cen_g, SDL_FLIP_NONE);
    }
```

- [ ] **Step 3: Build and run — verify full arm renders with sprites and grabber**

```bash
cmake --build Game/build --parallel && cd Game/build && ./game
```
Expected:
- PIVOT segment: metallic sprite, LED active on selected segment only
- EXTEND/BOTH segment: outer sleeve anchored at base joint, inner rod sliding visibly from tip back into sleeve
- Suction grabber: triangle+pivot circle at arm tip, oriented outward

- [ ] **Step 4: Commit**

```bash
git add Game/render.cpp
git commit -m "feat: render EXTEND/BOTH segments and suction grabber with sprites"
```

---

## Task 6 — Update `level1.level` to use standard segment lengths

**Files:**
- Modify: `Game/levels/level1.level`

Current file has `segment pivot 192` and `segment both 96`. Neither matches a standard size (32/48/64/80). `pivot 192` will trigger the red fallback line. Update to valid sizes and reposition the arm/object/target so gameplay is preserved.

- [ ] **Step 1: Replace the `arm:` block and adjust positions**

Replace the `arm:` section and position data with:

```
arm:
base 64 288
angle 0
segment pivot 64
segment both 64
```

Update `object:` and `target:` sections so the object is reachable:

```
object:
pos 320 380

target:
pos 480 512 64 32
```

- [ ] **Step 2: Build and run — verify the arm reaches the object and target zone**

```bash
cmake --build Game/build --parallel && cd Game/build && ./game
```
Expected: arm visible in the level, can grab and carry the object to the green target zone.

- [ ] **Step 3: Tweak positions if needed**

If the arm can't reach the object, adjust `object: pos` closer to the arm tip range. The pivot segment is 64 px. The BOTH segment extends from `MIN_SEG_LEN` (10 px) to `MAX_SEG_LEN` (384 px), giving a total reach of 64+10 = 74 px minimum to 64+384 = 448 px maximum.

- [ ] **Step 4: Commit**

```bash
git add Game/levels/level1.level
git commit -m "fix: update level1 segment lengths to standard sprite sizes"
```

---

## Self-Review Checklist

| Spec requirement | Covered by |
|-----------------|-----------|
| PIVOT → `arm_segment_{size}` at native size, rotated | Task 4 |
| EXTEND/BOTH → `arm_outer` + `arm_inner` overlap | Task 5 |
| Inner behind, outer on top | Task 5 (draw order) |
| LED `_active` only on `is_active && i == arm.active_segment` | Tasks 4 + 5 |
| Suction grabber at tip, rotated outward | Task 5 |
| Level lengths updated to valid standard sizes | Task 6 |
| `free_textures` cleans up all new textures | Task 2 |
| Fallback for invalid segment lengths | Task 4 (red line) |
