// Game/render.cpp
#include "render.h"
#include "gravity.h"
#include <SDL3_image/SDL_image.h>
#include <vector>
#include <cmath>

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

    return out.tile && out.bg && out.object && out.arm[0].seg_active && out.grab_suction;
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
        render_arm(renderer, tex, level.arms[i], i == level.active_arm);
    }
}

static int arm_size_index(float len) {
    if (len == 32.0f) return 0;
    if (len == 48.0f) return 1;
    if (len == 64.0f) return 2;
    if (len == 80.0f) return 3;
    return -1;
}

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
            if (si < 0) {
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
        } else {
            if (si < 0) {
                SDL_SetRenderDrawColor(renderer, 200, 50, 50, 255);
                SDL_RenderLine(renderer, j0.x, j0.y, j1.x, j1.y);
                continue;
            }
            int esi = si;
            float ow = 48.0f + esi * 16.0f;
            float iw = 48.0f + esi * 16.0f;
            float oh = 20.0f;
            float ih = 12.0f;

            SDL_Texture* ti = tex.arm[esi].inner;
            if (ti) {
                SDL_FRect dst_i = { j1.x, j1.y - ih * 0.5f, iw, ih };
                SDL_FPoint cen_i = { 0.0f, ih * 0.5f };
                SDL_RenderTextureRotated(renderer, ti, nullptr, &dst_i,
                                         angle_deg + 180.0, &cen_i, SDL_FLIP_NONE);
            }

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
    }

    if (tex.grab_suction) {
        const Vec2& tip = joints.back();
        float last_dx = joints.back().x - joints[joints.size()-2].x;
        float last_dy = joints.back().y - joints[joints.size()-2].y;
        double grab_angle = std::atan2((double)last_dy, (double)last_dx)
                            * 180.0 / M_PI - 90.0;
        SDL_FRect dst_g = { tip.x - 12.0f, tip.y - 4.0f, 24.0f, 20.0f };
        SDL_FPoint cen_g = { 12.0f, 4.0f };
        SDL_RenderTextureRotated(renderer, tex.grab_suction, nullptr, &dst_g,
                                 grab_angle, &cen_g, SDL_FLIP_NONE);
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
