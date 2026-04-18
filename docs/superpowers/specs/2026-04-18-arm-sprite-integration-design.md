# Arm Sprite Integration Design
**Date:** 2026-04-18

## Goal
Replace the current placeholder line-drawing in `render_arm` with the pixel-art arm sprites stored in `Game/assets/arm/`. No stretching — sprites are rendered at native size; segment lengths in level files must match a standard size.

---

## Standard Sizes

| Variant | Segment length (px) | Sprite width |
|---------|--------------------:|--------------|
| `_sm`   | 32                  | 32 (segment), 48 (inner/outer) |
| `_md`   | 48                  | 48 (segment), 64 (inner/outer) |
| `_lg`   | 64                  | 64 (segment), 80 (inner/outer) |
| `_xl`   | 80                  | 80 (segment), 96 (inner/outer) |

`segment.length` must be exactly 32, 48, 64, or 80. Any other value is a level-authoring error and falls back to solid-colour rendering.

---

## Sprite → Segment Type Mapping

### PIVOT segment → `arm_segment_{size}`
- Rendered at native size, rotated by the cumulative joint angle.
- Sprite centre placed at the midpoint between `joint[i]` and `joint[i+1]`.
- LED variant: `_active` when `is_active && i == arm.active_segment`, else `_inactive`.

### EXTEND / BOTH segment → `arm_outer_{size}` + `arm_inner_{size}`
- **Outer sleeve** anchored at `joint[i]` (base joint), oriented along segment direction, rendered at native size.
- **Inner rod** rendered with its pivot end at `joint[i+1]` (tip joint), pointing back toward `joint[i]`, rendered at native size.
- As the segment contracts the inner rod slides into the outer sleeve; as it extends it slides out. Overlap produces the telescoping effect naturally.
- Draw order: inner first (behind), outer on top.
- LED variant (outer sleeve only): `_active` / `_inactive` same rule as PIVOT.

### Suction grabber → `arm_grab_suction`
- Always rendered at `joint[last]` (the arm tip).
- Rotated so the flat rubber pad faces outward along the final segment's direction.
- Pivot joint circle at the sprite tip sits exactly on `joint[last]`.

---

## LED State Rule
`_active` sprite variant → `is_active == true && i == arm.active_segment`
`_inactive` sprite variant → all other segments (including segments on inactive arms)

---

## Code Changes

### `render.h`
- Add arm texture fields to `Textures`:
  - `arm_segment[4]` — one per size variant (sm/md/lg/xl), active + inactive = 8 textures
  - `arm_outer[4]` — active + inactive = 8 textures
  - `arm_inner[4]` — 4 textures (no LED)
  - `arm_grab_suction` — 1 texture

### `render.cpp`
- `load_textures`: load all arm sprites above.
- `free_textures`: destroy all new textures.
- `render_arm`: rewrite to use `SDL_RenderTextureRotated` per segment:
  1. Call `compute_fk` to get joint positions.
  2. For each segment, pick variant from `segment.length`.
  3. Dispatch on `segment.type` to draw PIVOT or EXTEND pair.
  4. After all segments, draw suction grabber at tip.

---

## Out of Scope
- Animating the grabber open/close state.
- Adding new segment types.
- Modifying physics or movement code.
