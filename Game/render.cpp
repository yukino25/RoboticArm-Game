// Game/render.cpp
#include "render.h"
#include "gravity.h"
#include <vector>

static SDL_FRect to_sdl(const Rect& r) { return {r.x, r.y, r.w, r.h}; }

void render_level(SDL_Renderer* renderer, const Level& level, bool won) {
    // Tiles
    SDL_SetRenderDrawColor(renderer, 100, 100, 110, 255);
    for (int y = 0; y < (int)GAME_HEIGHT; y++) {
        for (int x = 0; x < (int)GAME_WIDTH; x++) {
            if (level.tiles[y * GAME_WIDTH + x] == TileType::SOLID) {
                SDL_FRect r{x * BLOCK_SIZE, y * BLOCK_SIZE, BLOCK_SIZE, BLOCK_SIZE};
                SDL_RenderFillRect(renderer, &r);
            }
        }
    }

    // Target zone outline
    SDL_SetRenderDrawColor(renderer, won ? 0 : 0, won ? 220 : 180, won ? 100 : 0, 255);
    SDL_FRect tz = to_sdl(level.target_zone);
    SDL_RenderRect(renderer, &tz);

    // Arms
    for (int i = 0; i < (int)level.arms.size(); i++) {
        render_arm(renderer, level.arms[i], i == level.active_arm);
    }
}

void render_arm(SDL_Renderer* renderer, const Arm& arm, bool is_active) {
    std::vector<Vec2> joints = compute_fk(arm);

    for (int i = 0; i < (int)arm.segments.size(); i++) {
        bool selected = is_active && (i == arm.active_segment);

        if (selected)       SDL_SetRenderDrawColor(renderer, 255, 220,   0, 255);
        else if (is_active) SDL_SetRenderDrawColor(renderer, 180, 180, 255, 255);
        else                SDL_SetRenderDrawColor(renderer, 140, 140, 140, 255);

        float x1 = joints[i].x,   y1 = joints[i].y;
        float x2 = joints[i+1].x, y2 = joints[i+1].y;
        // Thick line via parallel offsets
        for (int d = -1; d <= 1; d++) {
            SDL_RenderLine(renderer, x1 + d, y1,     x2 + d, y2);
            SDL_RenderLine(renderer, x1,     y1 + d, x2,     y2 + d);
        }
    }

    // Joints as small white squares
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    for (const auto& j : joints) {
        SDL_FRect dot{j.x - 3.0f, j.y - 3.0f, 6.0f, 6.0f};
        SDL_RenderFillRect(renderer, &dot);
    }
}

void render_object(SDL_Renderer* renderer, const Object& obj) {
    SDL_FRect r{obj.x, obj.y, OBJ_W, OBJ_H};
    SDL_SetRenderDrawColor(renderer, 80, 140, 255, 255);
    SDL_RenderFillRect(renderer, &r);
    SDL_SetRenderDrawColor(renderer, 160, 200, 255, 255);
    SDL_RenderRect(renderer, &r);
}

void render_ui(SDL_Renderer* renderer, const GameState& gs) {
    const Arm& arm = gs.level.arms[gs.level.active_arm];
    constexpr float SQ = 8.0f, GAP = 3.0f;
    float ox = 6.0f, oy = 6.0f;

    for (int i = 0; i < (int)arm.segments.size(); i++) {
        bool sel = (arm.active_segment == i);
        SDL_SetRenderDrawColor(renderer,
            sel ? 255 : 80,
            sel ? 220 : 80,
            sel ?   0 : 80,
            255);
        SDL_FRect sq{ox + i * (SQ + GAP), oy, SQ, SQ};
        SDL_RenderFillRect(renderer, &sq);
    }

    if (gs.won) {
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, 0, 255, 0, 55);
        SDL_FRect full{0, 0, BLOCK_SIZE * GAME_WIDTH, BLOCK_SIZE * GAME_HEIGHT};
        SDL_RenderFillRect(renderer, &full);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
    }
}
