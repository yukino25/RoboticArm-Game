// Game/render.h
#pragma once
#include <SDL3/SDL.h>
#include "types.h"

// ── Game textures ────────────────────────────────────────────────────────────

struct ArmTexSet {
    SDL_Texture* seg_active     = nullptr;
    SDL_Texture* seg_inactive   = nullptr;
    SDL_Texture* outer_active   = nullptr;
    SDL_Texture* outer_inactive = nullptr;
    SDL_Texture* inner          = nullptr;
};

struct Textures {
    SDL_Texture* tile    = nullptr;
    SDL_Texture* bg      = nullptr;
    SDL_Texture* object  = nullptr;
    ArmTexSet    arm[4]{};
    SDL_Texture* grab_suction = nullptr;
};

// ── GUI textures ─────────────────────────────────────────────────────────────

struct GuiTextures {
    // HUD bar
    SDL_Texture* hud_bg     = nullptr;
    SDL_Texture* btn_left   = nullptr;
    SDL_Texture* btn_right  = nullptr;
    SDL_Texture* btn_pause  = nullptr;
    SDL_Texture* btn_play   = nullptr;
    SDL_Texture* btn_menu   = nullptr;
    SDL_Texture* dot        = nullptr;
    SDL_Texture* dot_active = nullptr;
    // Menu overlay
    SDL_Texture* menu_resume  = nullptr;
    SDL_Texture* menu_restart = nullptr;
    SDL_Texture* menu_choose  = nullptr;
    SDL_Texture* menu_back    = nullptr;
    // Level select (5 levels)
    SDL_Texture* lbtn[5]{};
    SDL_Texture* lbtn_active[5]{};
};

// ── Overlay state ─────────────────────────────────────────────────────────────

enum class Overlay { NONE, MENU, LEVEL_SELECT };

// ── Overlay layout constants (render-space pixels) ────────────────────────────

constexpr float MENU_PW = 200.0f, MENU_PH = 140.0f;
constexpr float MENU_PX = (BLOCK_SIZE * GAME_WIDTH  - MENU_PW) / 2.0f;
constexpr float MENU_PY = (BLOCK_SIZE * GAME_HEIGHT - MENU_PH) / 2.0f;
constexpr float MENU_BTN_X       = MENU_PX + 20.0f;
constexpr float MENU_BTN_W       = 160.0f;
constexpr float MENU_BTN_H       = 28.0f;
constexpr float MENU_BTN_RESUME_Y  = MENU_PY + 18.0f;
constexpr float MENU_BTN_RESTART_Y = MENU_PY + 56.0f;
constexpr float MENU_BTN_CHOOSE_Y  = MENU_PY + 94.0f;

constexpr float LVLS_PW = 220.0f, LVLS_PH = 238.0f;
constexpr float LVLS_PX = (BLOCK_SIZE * GAME_WIDTH  - LVLS_PW) / 2.0f;
constexpr float LVLS_PY = (BLOCK_SIZE * GAME_HEIGHT - LVLS_PH) / 2.0f;
constexpr float LVLS_BTN_X      = LVLS_PX + 30.0f;
constexpr float LVLS_BTN_W      = 160.0f;
constexpr float LVLS_BTN_H      = 28.0f;
constexpr float LVLS_BTN_BACK_Y  = LVLS_PY + 10.0f;
constexpr float LVLS_BTN_FIRST_Y = LVLS_PY + 48.0f;
constexpr float LVLS_BTN_STEP    = 36.0f;

// ── Function declarations ─────────────────────────────────────────────────────

bool load_textures(SDL_Renderer* renderer, Textures& out);
void free_textures(Textures& t);

bool load_gui_textures(SDL_Renderer* renderer, GuiTextures& out);
void free_gui_textures(GuiTextures& t);

void render_level(SDL_Renderer* renderer, const Textures& tex, const Level& level, bool won);
void render_arm(SDL_Renderer* renderer, const Textures& tex, const Arm& arm, bool is_active);
void render_object(SDL_Renderer* renderer, const Textures& tex, const Object& obj);
void render_ui(SDL_Renderer* renderer, const GameState& gs);
void render_hud(SDL_Renderer* renderer, const GuiTextures& gui,
                int active_level, int num_levels, bool paused);
void render_overlay(SDL_Renderer* renderer, const GuiTextures& gui,
                    Overlay overlay, int active_level, int num_levels);
