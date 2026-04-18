// Game/render.h
#pragma once
#include <SDL3/SDL.h>
#include "types.h"

struct ArmTexSet {
    SDL_Texture* seg_active     = nullptr;   // arm_segment_{size}_active
    SDL_Texture* seg_inactive   = nullptr;   // arm_segment_{size}_inactive
    SDL_Texture* outer_active   = nullptr;   // arm_outer_{size}_active
    SDL_Texture* outer_inactive = nullptr;   // arm_outer_{size}_inactive
    SDL_Texture* inner          = nullptr;   // arm_inner_{size}
};

struct Textures {
    SDL_Texture* tile    = nullptr;
    SDL_Texture* bg      = nullptr;
    SDL_Texture* object  = nullptr;
    ArmTexSet    arm[4]{};                   // [0]=sm [1]=md [2]=lg [3]=xl
    SDL_Texture* grab_suction = nullptr;
};

// Load all game textures. Returns false and logs on failure.
bool load_textures(SDL_Renderer* renderer, Textures& out);
void free_textures(Textures& t);

void render_level(SDL_Renderer* renderer, const Textures& tex, const Level& level, bool won);
void render_arm(SDL_Renderer* renderer, const Textures& tex, const Arm& arm, bool is_active);
void render_object(SDL_Renderer* renderer, const Textures& tex, const Object& obj);
void render_ui(SDL_Renderer* renderer, const GameState& gs);
