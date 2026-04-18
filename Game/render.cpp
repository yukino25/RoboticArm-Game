// Game/render.cpp
#include "render.h"
#include "gravity.h"
#include <SDL3_image/SDL_image.h>
#include <vector>

static SDL_FRect to_sdl(const Rect& r) { return {r.x, r.y, r.w, r.h}; }

// ---- Texture loading --------------------------------------------------------

static SDL_Texture* load_tex(SDL_Renderer* r, const char* path) {
    SDL_Texture* t = IMG_LoadTexture(r, path);
    if (!t) SDL_Log("Failed to load texture %s: %s", path, SDL_GetError());
    return t;
}

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

// ---- Rendering --------------------------------------------------------------

void render_level(SDL_Renderer* renderer, const Textures& tex, const Level& level, bool won) {
    // Background — stretched to fill the whole viewport
    if (tex.bg) {
        SDL_FRect full{0, 0, BLOCK_SIZE * GAME_WIDTH, BLOCK_SIZE * GAME_HEIGHT};
        SDL_RenderTexture(renderer, tex.bg, nullptr, &full);
    }

    // Tiles
    for (int y = 0; y < (int)GAME_HEIGHT; y++) {
        for (int x = 0; x < (int)GAME_WIDTH; x++) {
            if (level.tiles[y * GAME_WIDTH + x] == TileType::SOLID) {
                SDL_FRect r{x * BLOCK_SIZE, y * BLOCK_SIZE, BLOCK_SIZE, BLOCK_SIZE};
                if (tex.tile) {
                    SDL_RenderTexture(renderer, tex.tile, nullptr, &r);
                } else {
                    SDL_SetRenderDrawColor(renderer, 100, 100, 110, 255);
                    SDL_RenderFillRect(renderer, &r);
                }
            }
        }
    }

    // Target zone outline
    SDL_SetRenderDrawColor(renderer, 0, won ? 220 : 180, won ? 100 : 0, 255);
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
        for (int d = -1; d <= 1; d++) {
            SDL_RenderLine(renderer, x1 + d, y1,     x2 + d, y2);
            SDL_RenderLine(renderer, x1,     y1 + d, x2,     y2 + d);
        }
    }

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    for (const auto& j : joints) {
        SDL_FRect dot{j.x - 3.0f, j.y - 3.0f, 6.0f, 6.0f};
        SDL_RenderFillRect(renderer, &dot);
    }
}

void render_object(SDL_Renderer* renderer, const Textures& tex, const Object& obj) {
    SDL_FRect r{obj.x, obj.y, OBJ_W, OBJ_H};
    if (tex.object) {
        SDL_RenderTexture(renderer, tex.object, nullptr, &r);
    } else {
        SDL_SetRenderDrawColor(renderer, 80, 140, 255, 255);
        SDL_RenderFillRect(renderer, &r);
        SDL_SetRenderDrawColor(renderer, 160, 200, 255, 255);
        SDL_RenderRect(renderer, &r);
    }
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
