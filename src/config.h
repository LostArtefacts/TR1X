#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    SCREENSHOT_FORMAT_JPEG,
    SCREENSHOT_FORMAT_PNG,
} SCREENSHOT_FORMAT;

typedef enum {
    T1M_BL_TOP_LEFT = 0,
    T1M_BL_TOP_CENTER = 1,
    T1M_BL_TOP_RIGHT = 2,
    T1M_BL_BOTTOM_LEFT = 3,
    T1M_BL_BOTTOM_CENTER = 4,
    T1M_BL_BOTTOM_RIGHT = 5,
} T1M_BAR_LOCATION;

typedef enum {
    T1M_BC_GOLD = 0,
    T1M_BC_BLUE = 1,
    T1M_BC_GREY = 2,
    T1M_BC_RED = 3,
    T1M_BC_SILVER = 4,
    T1M_BC_GREEN = 5,
    T1M_BC_GOLD2 = 6,
    T1M_BC_BLUE2 = 7,
    T1M_BC_PINK = 8,
} T1M_BAR_COLOR;

typedef enum {
    T1M_BSM_DEFAULT = 0,
    T1M_BSM_FLASHING_OR_DEFAULT = 1,
    T1M_BSM_FLASHING_ONLY = 2,
    T1M_BSM_ALWAYS = 3,
    T1M_BSM_NEVER = 4,
} T1M_BAR_SHOW_MODE;

typedef struct {
    bool disable_healing_between_levels;
    bool disable_medpacks;
    bool disable_magnums;
    bool disable_uzis;
    bool disable_shotgun;
    bool enable_enemy_healthbar;
    bool enable_enhanced_look;
    bool enable_numeric_keys;
    bool enable_shotgun_flash;
    bool enable_cheats;
    bool enable_tr3_sidesteps;
    bool enable_braid;
    bool enable_compass_stats;
    bool enable_timer_in_inventory;
    bool enable_smooth_bars;
    int8_t healthbar_showing_mode;
    int8_t healthbar_location;
    int8_t healthbar_color;
    int8_t airbar_showing_mode;
    int8_t airbar_location;
    int8_t airbar_color;
    int8_t enemy_healthbar_location;
    int8_t enemy_healthbar_color;
    bool fix_tihocan_secret_sound;
    bool fix_pyramid_secret_trigger;
    bool fix_secrets_killing_music;
    bool fix_descending_glitch;
    bool fix_wall_jump_glitch;
    bool fix_bridge_collision;
    bool fix_qwop_glitch;
    int32_t fov_value;
    bool fov_vertical;
    bool disable_demo;
    bool disable_fmv;
    bool disable_cine;
    bool disable_music_in_menu;
    int32_t resolution_width;
    int32_t resolution_height;
    bool enable_xbox_one_controller;
    float brightness;
    bool enable_round_shadow;
    bool enable_3d_pickups;

    struct {
        int32_t layout;
    } input;

    struct {
        uint32_t perspective : 1;
        uint32_t bilinear : 1;
        uint32_t fps_counter : 1;
    } render_flags;

    struct {
        double text_scale;
        double bar_scale;
    } ui;

    int32_t sound_volume;
    int32_t music_volume;

    SCREENSHOT_FORMAT screenshot_format;
} CONFIG;

extern CONFIG g_Config;

bool Config_ReadFromJSON(const char *json);
bool Config_Read();
