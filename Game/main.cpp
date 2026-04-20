// Game/main.cpp
#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3_image/SDL_image.h>
#include <algorithm>
#include <filesystem>
#include "types.h"
#include "gravity.h"
#include "movement.h"
#include "level.h"
#include "render.h"

#define SDL_WINDOW_WIDTH  (int)(BLOCK_SIZE * GAME_WIDTH)
#define SDL_WINDOW_HEIGHT (int)(BLOCK_SIZE * GAME_HEIGHT)

constexpr int NUM_LEVELS = 5;

struct AppState {
    SDL_Window*   window   = nullptr;
    SDL_Renderer* renderer = nullptr;
    Textures    tex{};
    GuiTextures gui{};
    GameState   gs{};
    Vec2   prev_tip{};
    Uint64 prev_ticks = 0;
    bool    paused  = false;
    Overlay overlay = Overlay::NONE;
};

static void load_game_level(AppState* state, int idx) {
    char path[64];
    SDL_snprintf(path, sizeof(path), "levels/level%d.level", idx + 1);
    auto loaded = load_level(path);
    if (!loaded) { SDL_Log("Failed to load %s", path); return; }
    state->gs.level       = *loaded;
    state->gs.won         = false;
    state->gs.level_index = idx;
    state->paused         = false;
    state->overlay        = Overlay::NONE;
    state->prev_tip       = arm_tip(state->gs.level.arms[0]);
}

SDL_AppResult SDL_AppInit(void** appstate, int argc, char* argv[]) {
    SDL_SetAppMetadata("Robotic Arm Game", "0.1", "com.example.robotic-arm");

    // Ensure relative asset paths work regardless of how/where the exe is launched.
    const char* base = SDL_GetBasePath();
    if (base) {
        std::error_code ec;
        std::filesystem::current_path(base, ec);
    }

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    auto* state = new AppState();
    *appstate = state;

    if (!SDL_CreateWindowAndRenderer("Robotic Arm Game",
            SDL_WINDOW_WIDTH, SDL_WINDOW_HEIGHT,
            SDL_WINDOW_RESIZABLE, &state->window, &state->renderer)) {
        SDL_Log("SDL_CreateWindowAndRenderer failed: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    SDL_SetRenderLogicalPresentation(state->renderer,
        SDL_WINDOW_WIDTH, SDL_WINDOW_HEIGHT, SDL_LOGICAL_PRESENTATION_LETTERBOX);

    if (!load_textures(state->renderer, state->tex))
        SDL_Log("Warning: some game textures failed to load");
    if (!load_gui_textures(state->renderer, state->gui))
        SDL_Log("Warning: some GUI textures failed to load");

    load_game_level(state, 0);
    state->prev_ticks = SDL_GetTicks();
    return SDL_APP_CONTINUE;
}

// ---- hit-test helpers -------------------------------------------------------

static bool hit(float mx, float my, float x, float y, float w, float h) {
    return mx >= x && mx < x + w && my >= y && my < y + h;
}

SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event) {
    auto* state = static_cast<AppState*>(appstate);
    GameState& gs = state->gs;

    if (event->type == SDL_EVENT_QUIT) return SDL_APP_SUCCESS;

    // ---- Mouse --------------------------------------------------------------
    if (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
        float rx, ry;
        SDL_RenderCoordinatesFromWindow(state->renderer,
            event->button.x, event->button.y, &rx, &ry);

        // ── Overlay interactions ──────────────────────────────────────────
        if (state->overlay == Overlay::MENU) {
            if (hit(rx, ry, MENU_BTN_X, MENU_BTN_RESUME_Y,  MENU_BTN_W, MENU_BTN_H)) {
                state->paused  = false;
                state->overlay = Overlay::NONE;
            } else if (hit(rx, ry, MENU_BTN_X, MENU_BTN_RESTART_Y, MENU_BTN_W, MENU_BTN_H)) {
                load_game_level(state, gs.level_index);
            } else if (hit(rx, ry, MENU_BTN_X, MENU_BTN_CHOOSE_Y,  MENU_BTN_W, MENU_BTN_H)) {
                state->overlay = Overlay::LEVEL_SELECT;
            } else if (!hit(rx, ry, MENU_PX, MENU_PY, MENU_PW, MENU_PH)) {
                // Click outside panel — close
                state->paused  = false;
                state->overlay = Overlay::NONE;
            }
            return SDL_APP_CONTINUE;
        }

        if (state->overlay == Overlay::LEVEL_SELECT) {
            if (hit(rx, ry, LVLS_BTN_X, LVLS_BTN_BACK_Y, 120, LVLS_BTN_H)) {
                state->overlay = Overlay::MENU;
            } else {
                for (int i = 0; i < NUM_LEVELS; i++) {
                    float by = LVLS_BTN_FIRST_Y + i * LVLS_BTN_STEP;
                    if (hit(rx, ry, LVLS_BTN_X, by, LVLS_BTN_W, LVLS_BTN_H)) {
                        load_game_level(state, i);
                        break;
                    }
                }
                if (!hit(rx, ry, LVLS_PX, LVLS_PY, LVLS_PW, LVLS_PH)) {
                    state->paused  = false;
                    state->overlay = Overlay::NONE;
                }
            }
            return SDL_APP_CONTINUE;
        }

        // ── HUD interactions (no overlay open) ────────────────────────────
        if (ry < BLOCK_SIZE) {
            if (hit(rx, ry, 8, 4, 24, 24)) {                          // menu
                state->paused  = true;
                state->overlay = Overlay::MENU;
            } else if (hit(rx, ry, 736, 4, 24, 24)) {                 // pause/play
                state->paused = !state->paused;
            }
            return SDL_APP_CONTINUE;
        }
    }

    // ---- Keyboard -----------------------------------------------------------
    if (event->type == SDL_EVENT_KEY_DOWN && !event->key.repeat) {
        Level&  level = gs.level;
        Arm&    arm   = level.arms[level.active_arm];
        Object& obj   = level.object;

        switch (event->key.scancode) {
            case SDL_SCANCODE_ESCAPE:
                if (state->overlay != Overlay::NONE) {
                    state->paused  = false;
                    state->overlay = Overlay::NONE;
                } else {
                    state->paused  = true;
                    state->overlay = Overlay::MENU;
                }
                break;
            case SDL_SCANCODE_Q:
                if (state->overlay == Overlay::NONE) cycle_selection(gs, -1);
                break;
            case SDL_SCANCODE_E:
                if (state->overlay == Overlay::NONE) cycle_selection(gs, +1);
                break;
            case SDL_SCANCODE_R:
                load_game_level(state, gs.level_index);
                break;
            case SDL_SCANCODE_SPACE:
                if (state->overlay != Overlay::NONE) break;
                if (gs.won) {
                    load_game_level(state, (gs.level_index + 1) % NUM_LEVELS);
                } else if (!state->paused) {
                    if (obj.grabbed) {
                        Uint64 now = SDL_GetTicks();
                        float  dt  = (now - state->prev_ticks) / 1000.0f;
                        Vec2 cur_tip = arm_tip(arm);
                        Vec2 tip_vel{0.0f, 0.0f};
                        if (dt > 0.0f) {
                            tip_vel.x = (cur_tip.x - state->prev_tip.x) / dt;
                            tip_vel.y = (cur_tip.y - state->prev_tip.y) / dt;
                        }
                        release_object(obj, tip_vel);
                    } else if (can_grab(obj, arm)) {
                        grab_object(obj);
                    }
                }
                break;
            default: break;
        }
    }

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void* appstate) {
    auto* state = static_cast<AppState*>(appstate);
    GameState& gs = state->gs;
    Level& level  = gs.level;

    Uint64 now = SDL_GetTicks();
    float  dt  = std::min((now - state->prev_ticks) / 1000.0f, 0.05f);
    state->prev_ticks = now;

    Arm& active_arm = level.arms[level.active_arm];
    state->prev_tip = arm_tip(active_arm);

    if (!state->paused && state->overlay == Overlay::NONE && !gs.won) {
        const bool* keys = SDL_GetKeyboardState(nullptr);
        float delta_angle = 0, delta_extend = 0, delta_track = 0;
        if (keys[SDL_SCANCODE_A])  delta_angle  = -PIVOT_SPEED  * dt;
        if (keys[SDL_SCANCODE_D])  delta_angle  =  PIVOT_SPEED  * dt;
        if (keys[SDL_SCANCODE_W]) { delta_extend =  EXTEND_SPEED * dt; delta_track =  TRACK_SPEED * dt; }
        if (keys[SDL_SCANCODE_S]) { delta_extend = -EXTEND_SPEED * dt; delta_track = -TRACK_SPEED * dt; }
        apply_movement(active_arm, delta_angle, delta_extend, delta_track,
                       level.tiles, level.arms, level.active_arm);

        Object& obj = level.object;
        if (obj.grabbed) {
            Vec2 tip = grabber_tip(active_arm);
            obj.x  = tip.x - OBJ_W / 2;
            obj.y  = tip.y - OBJ_H / 2;
            obj.vx = obj.vy = 0.0f;
        } else {
            update_object(obj, dt);
            resolve_tile_collision(obj, level.tiles, OBJ_W, OBJ_H);
        }

        float cx = level.object.x + OBJ_W / 2;
        float cy = level.object.y + OBJ_H / 2;
        const Rect& tz = level.target_zone;
        if (cx >= tz.x && cx <= tz.x + tz.w && cy >= tz.y && cy <= tz.y + tz.h)
            gs.won = true;
    }

    SDL_SetRenderDrawColor(state->renderer, 30, 30, 35, 255);
    SDL_RenderClear(state->renderer);
    render_level(state->renderer, state->tex, level, gs.won);
    render_object(state->renderer, state->tex, level.object);
    render_ui(state->renderer, gs);
    render_hud(state->renderer, state->gui, gs.level_index, NUM_LEVELS, state->paused);
    render_overlay(state->renderer, state->gui, state->overlay, gs.level_index, NUM_LEVELS);
    SDL_RenderPresent(state->renderer);

    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void* appstate, SDL_AppResult result) {
    auto* state = static_cast<AppState*>(appstate);
    free_textures(state->tex);
    free_gui_textures(state->gui);
    delete state;
}
