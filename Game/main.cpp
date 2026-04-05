// Game/main.cpp
#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <algorithm>
#include "types.h"
#include "gravity.h"
#include "movement.h"
#include "level.h"
#include "render.h"

#define SDL_WINDOW_WIDTH  (int)(BLOCK_SIZE * GAME_WIDTH)
#define SDL_WINDOW_HEIGHT (int)(BLOCK_SIZE * GAME_HEIGHT)

struct AppState {
    SDL_Window*   window   = nullptr;
    SDL_Renderer* renderer = nullptr;
    GameState gs{};
    Vec2   prev_tip{};
    Uint64 prev_ticks = 0;
};

SDL_AppResult SDL_AppInit(void** appstate, int argc, char* argv[]) {
    SDL_SetAppMetadata("Robotic Arm Game", "0.1", "com.example.robotic-arm");

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

    state->gs.level       = make_level_1();
    state->gs.won         = false;
    state->gs.level_index = 0;
    state->prev_tip       = arm_tip(state->gs.level.arms[0]);
    state->prev_ticks     = SDL_GetTicks();

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event) {
    auto* state = static_cast<AppState*>(appstate);
    GameState& gs = state->gs;

    if (event->type == SDL_EVENT_QUIT) return SDL_APP_SUCCESS;

    if (event->type == SDL_EVENT_KEY_DOWN && !event->key.repeat) {
        Level& level = gs.level;
        Arm&   arm   = level.arms[level.active_arm];
        Object& obj  = level.object;

        switch (event->key.scancode) {
            case SDL_SCANCODE_1: cycle_selection(gs, -1); break;
            case SDL_SCANCODE_2: cycle_selection(gs, +1); break;
            case SDL_SCANCODE_E: {
                if (obj.grabbed) {
                    Uint64 now = SDL_GetTicks();
                    float dt = (now - state->prev_ticks) / 1000.0f;
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
                break;
            }
            default: break;
        }
    }

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void* appstate) {
    auto* state = static_cast<AppState*>(appstate);
    GameState& gs = state->gs;
    Level& level  = gs.level;

    // Delta time
    Uint64 now = SDL_GetTicks();
    float dt = std::min((now - state->prev_ticks) / 1000.0f, 0.05f);
    state->prev_ticks = now;

    // Track tip position for velocity on release.
    // TODO Phase C: when two arms exist, track the grabbing arm's tip, not the active arm's.
    Arm& active_arm = level.arms[level.active_arm];
    state->prev_tip = arm_tip(active_arm);

    // Held-key input
    const bool* keys = SDL_GetKeyboardState(nullptr);
    float delta_angle  = 0.0f, delta_extend = 0.0f, delta_track = 0.0f;
    if (keys[SDL_SCANCODE_LEFT])  delta_angle  = -PIVOT_SPEED  * dt;
    if (keys[SDL_SCANCODE_RIGHT]) delta_angle  =  PIVOT_SPEED  * dt;
    if (keys[SDL_SCANCODE_UP])  { delta_extend =  EXTEND_SPEED * dt; delta_track =  TRACK_SPEED * dt; }
    if (keys[SDL_SCANCODE_DOWN]){ delta_extend = -EXTEND_SPEED * dt; delta_track = -TRACK_SPEED * dt; }

    apply_movement(active_arm, delta_angle, delta_extend, delta_track,
                   level.tiles, level.arms, level.active_arm);

    // Object update
    Object& obj = level.object;
    if (obj.grabbed) {
        Vec2 tip = arm_tip(active_arm);
        obj.x = tip.x - OBJ_W / 2;
        obj.y = tip.y - OBJ_H / 2;
        obj.vx = obj.vy = 0.0f;
    } else {
        update_object(obj, dt);
        resolve_tile_collision(obj, level.tiles, OBJ_W, OBJ_H);
    }

    // Win check
    if (!gs.won) {
        float cx = obj.x + OBJ_W / 2;
        float cy = obj.y + OBJ_H / 2;
        const Rect& tz = level.target_zone;
        if (cx >= tz.x && cx <= tz.x + tz.w && cy >= tz.y && cy <= tz.y + tz.h)
            gs.won = true;
    }

    // Render
    SDL_SetRenderDrawColor(state->renderer, 30, 30, 35, 255);
    SDL_RenderClear(state->renderer);
    render_level(state->renderer, level, gs.won);
    render_object(state->renderer, obj);
    render_ui(state->renderer, gs);
    SDL_RenderPresent(state->renderer);

    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void* appstate, SDL_AppResult result) {
    delete static_cast<AppState*>(appstate);
}
