// Game/render.h
#pragma once
#include <SDL3/SDL.h>
#include "types.h"

struct Textures {
    SDL_Texture* tile    = nullptr;   // solid tile (IndustrialTile_26.png — grey floor block)
    SDL_Texture* bg      = nullptr;   // background
    SDL_Texture* object  = nullptr;   // the grabbable object (Box1.png)
};

// Load all game textures. Returns false and logs on failure.
bool load_textures(SDL_Renderer* renderer, Textures& out);
void free_textures(Textures& t);

void render_level(SDL_Renderer* renderer, const Textures& tex, const Level& level, bool won);
void render_arm(SDL_Renderer* renderer, const Arm& arm, bool is_active);
void render_object(SDL_Renderer* renderer, const Textures& tex, const Object& obj);
void render_ui(SDL_Renderer* renderer, const GameState& gs);
