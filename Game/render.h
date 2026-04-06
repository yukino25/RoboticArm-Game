// Game/render.h
#pragma once
#include <SDL3/SDL.h>
#include "types.h"

void render_level(SDL_Renderer* renderer, const Level& level, bool won);
void render_arm(SDL_Renderer* renderer, const Arm& arm, bool is_active);
void render_object(SDL_Renderer* renderer, const Object& obj);
void render_ui(SDL_Renderer* renderer, const GameState& gs);
