#pragma once
#include "types.h"
#include <vector>

// Returns n+1 joint positions: joints[0]=base, joints[n]=tip
std::vector<Vec2> compute_fk(const Arm& arm);

// Convenience: returns arm tip position
Vec2 arm_tip(const Arm& arm);

// Integrate gravity + velocity for one frame. Does nothing if obj.grabbed.
void update_object(Object& obj, float dt);

// Resolve object AABB against solid tiles. Modifies obj pos/vel.
void resolve_tile_collision(Object& obj, const TileType* tiles, float obj_w, float obj_h);