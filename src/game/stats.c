#include "game/stats.h"

#include "game/draw.h"
#include "game/gameflow.h"
#include "game/input.h"
#include "game/output.h"
#include "game/savegame.h"
#include "game/screen.h"
#include "game/shell.h"
#include "game/text.h"
#include "global/vars.h"
#include "log.h"

#include <stdio.h>
#include <string.h>

#define PIERRE_ITEMS 3
#define SKATEKID_ITEMS 1
#define COWBOY_ITEMS 1
#define BALDY_ITEMS 1

static int32_t m_LevelPickups = 0;
static int32_t m_LevelKillables = 0;
static int32_t m_LevelSecrets = 0;
static bool m_KillableItems[MAX_ITEMS] = { 0 };
static bool m_IfKillable[O_NUMBER_OF] = { 0 };

int16_t m_PickupObjs[] = { O_PICKUP_ITEM1,   O_PICKUP_ITEM2,  O_KEY_ITEM1,
                           O_KEY_ITEM2,      O_KEY_ITEM3,     O_KEY_ITEM4,
                           O_PUZZLE_ITEM1,   O_PUZZLE_ITEM2,  O_PUZZLE_ITEM3,
                           O_PUZZLE_ITEM4,   O_GUN_ITEM,      O_SHOTGUN_ITEM,
                           O_MAGNUM_ITEM,    O_UZI_ITEM,      O_GUN_AMMO_ITEM,
                           O_SG_AMMO_ITEM,   O_MAG_AMMO_ITEM, O_UZI_AMMO_ITEM,
                           O_EXPLOSIVE_ITEM, O_MEDI_ITEM,     O_BIGMEDI_ITEM,
                           O_SCION_ITEM,     O_SCION_ITEM2,   O_LEADBAR_ITEM,
                           NO_ITEM };

// Pierre has special trigger check
int16_t m_KillableObjs[] = {
    O_WOLF,     O_BEAR,        O_BAT,      O_CROCODILE, O_ALLIGATOR,
    O_LION,     O_LIONESS,     O_PUMA,     O_APE,       O_RAT,
    O_VOLE,     O_DINOSAUR,    O_RAPTOR,   O_WARRIOR1,  O_WARRIOR2,
    O_WARRIOR3, O_CENTAUR,     O_MUMMY,    O_ABORTION,  O_DINO_WARRIOR,
    O_FISH,     O_LARSON,      O_SKATEKID, O_COWBOY,    O_BALDY,
    O_NATLA,    O_SCION_ITEM3, O_STATUE,   O_PODS,      O_BIG_POD,
    NO_ITEM
};

static void Stats_CheckTriggers(FLOOR_INFO **floor_array);

static void Stats_CheckTriggers(FLOOR_INFO **floor_array)
{
    uint32_t secrets = 0;

    for (int i = 0; i < g_RoomCount; i++) {
        ROOM_INFO *r = &g_RoomInfo[i];
        for (int x_floor = 0; x_floor < r->x_size; x_floor++) {
            for (int y_floor = 0; y_floor < r->y_size; y_floor++) {

                if (x_floor == 0 || x_floor == r->x_size - 1) {
                    if (y_floor == 0 || y_floor == r->y_size - 1) {
                        continue;
                    }
                }

                FLOOR_INFO *floor =
                    &floor_array[i][x_floor + y_floor * r->x_size];

                if (!floor->index) {
                    continue;
                }

                int16_t *data = &g_FloorData[floor->index];
                int16_t type;
                int16_t trigger;
                int16_t trig_flags;
                int16_t trig_type;
                do {
                    type = *data++;

                    switch (type & DATA_TYPE) {
                    case FT_TILT:
                    case FT_ROOF:
                    case FT_DOOR:
                        data++;
                        break;

                    case FT_LAVA:
                        break;

                    case FT_TRIGGER:
                        trig_flags = *data;
                        data++;
                        trig_type = (type >> 8) & 0x3F;
                        do {
                            trigger = *data++;
                            if (TRIG_BITS(trigger) == TO_SECRET) {
                                int16_t number = trigger & VALUE_BITS;
                                if (!(secrets & (1 << number))) {
                                    secrets |= (1 << number);
                                    m_LevelSecrets++;
                                }
                            }
                            if (TRIG_BITS(trigger) != TO_OBJECT) {
                                if (TRIG_BITS(trigger) == TO_CAMERA) {
                                    trigger = *data++;
                                }
                            } else {
                                int16_t idx = trigger & VALUE_BITS;
                                ITEM_INFO *item = &g_Items[idx];

                                // Add Pierre pickup and kills if oneshot
                                if (item->object_number == O_PIERRE
                                    && trig_flags & IF_ONESHOT
                                    && !m_KillableItems[idx]) {
                                    m_KillableItems[idx] = true;
                                    m_LevelPickups += PIERRE_ITEMS;
                                    m_LevelKillables += 1;
                                }

                                // Add killable if object triggered
                                if (m_IfKillable[item->object_number]
                                    && !m_KillableItems[idx]) {
                                    m_KillableItems[idx] = true;
                                    m_LevelKillables += 1;

                                    // Add mercenary pickups
                                    if (item->object_number == O_SKATEKID) {
                                        m_LevelPickups += SKATEKID_ITEMS;
                                    }
                                    if (item->object_number == O_COWBOY) {
                                        m_LevelPickups += COWBOY_ITEMS;
                                    }
                                    if (item->object_number == O_BALDY) {
                                        m_LevelPickups += BALDY_ITEMS;
                                    }
                                }
                            }
                        } while (!(trigger & END_BIT));
                        break;
                    }
                } while (!(type & END_BIT));
            }
        }
    }
}

void Stats_Init()
{
    for (int i = 0; m_KillableObjs[i] != NO_ITEM; i++) {
        m_IfKillable[m_KillableObjs[i]] = true;
    }
}

void Stats_CalculateStats(int32_t uninit_item_count, FLOOR_INFO **floor_array)
{
    // Clear old values
    m_LevelPickups = 0;
    m_LevelKillables = 0;
    m_LevelSecrets = 0;
    memset(&m_KillableItems, 0, sizeof(m_KillableItems));

    if (uninit_item_count) {
        if (uninit_item_count > MAX_ITEMS) {
            Shell_ExitSystem(
                "Stats_GetPickupCount(): Too Many g_Items being Loaded!!");
            return;
        }

        for (int i = 0; i < uninit_item_count; i++) {
            ITEM_INFO *item = &g_Items[i];

            if (item->object_number < 0 || item->object_number >= O_NUMBER_OF) {
                Shell_ExitSystemFmt(
                    "Stats_GetPickupCount(): Bad Object number (%d) on Item %d",
                    item->object_number, i);
            }

            // Calculate number of pickups in a level
            for (int j = 0; m_PickupObjs[j] != NO_ITEM; j++) {
                if (item->object_number == m_PickupObjs[j]) {
                    m_LevelPickups++;
                }
            }
        }
    }

    // Check triggers for special pickups / killables
    Stats_CheckTriggers(floor_array);
}

int32_t Stats_GetPickups()
{
    return m_LevelPickups;
}

int32_t Stats_GetKillables()
{
    return m_LevelKillables;
}

int32_t Stats_GetSecrets()
{
    return m_LevelSecrets;
}

void Stats_LevelEnd(int32_t level_num)
{
    char string[100];
    char time_str[100];
    TEXTSTRING *txt;

    Text_RemoveAll();

    // heading
    sprintf(string, "%s", g_GameFlow.levels[level_num].level_title);
    txt = Text_Create(0, -50, string);
    Text_CentreH(txt, 1);
    Text_CentreV(txt, 1);

    // time taken
    int32_t seconds = g_GameInfo.timer / 30;
    int32_t hours = seconds / 3600;
    int32_t minutes = (seconds / 60) % 60;
    seconds %= 60;
    if (hours) {
        sprintf(
            time_str, "%d:%d%d:%d%d", hours, minutes / 10, minutes % 10,
            seconds / 10, seconds % 10);
    } else {
        sprintf(time_str, "%d:%d%d", minutes, seconds / 10, seconds % 10);
    }
    sprintf(string, g_GameFlow.strings[GS_STATS_TIME_TAKEN_FMT], time_str);
    txt = Text_Create(0, 70, string);
    Text_CentreH(txt, 1);
    Text_CentreV(txt, 1);

    // secrets
    int32_t secrets_taken = 0;
    int32_t secrets_total = MAX_SECRETS;
    do {
        if (g_GameInfo.secrets & 1) {
            secrets_taken++;
        }
        g_GameInfo.secrets >>= 1;
        secrets_total--;
    } while (secrets_total);
    sprintf(
        string, g_GameFlow.strings[GS_STATS_SECRETS_FMT], secrets_taken,
        g_GameFlow.levels[level_num].secrets);
    txt = Text_Create(0, 40, string);
    Text_CentreH(txt, 1);
    Text_CentreV(txt, 1);

    // pickups
    sprintf(
        string, g_GameFlow.strings[GS_STATS_PICKUPS_FMT], g_GameInfo.pickups,
        g_GameFlow.levels[level_num].pickups);
    txt = Text_Create(0, 10, string);
    Text_CentreH(txt, 1);
    Text_CentreV(txt, 1);

    // kills
    sprintf(
        string, g_GameFlow.strings[GS_STATS_KILLS_FMT], g_GameInfo.kills,
        g_GameFlow.levels[level_num].kills);
    txt = Text_Create(0, -20, string);
    Text_CentreH(txt, 1);
    Text_CentreV(txt, 1);

    Output_FadeToSemiBlack(true);
    // wait till a skip key is pressed
    do {
        if (g_ResetFlag) {
            break;
        }
        Output_InitialisePolyList();
        Draw_DrawScene(false);
        Input_Update();
        Text_Draw();
        Output_DumpScreen();
    } while (!g_InputDB.select && !g_InputDB.deselect);

    Output_FadeToBlack(false);
    Text_RemoveAll();

    // finish fading
    while (Output_FadeIsAnimating()) {
        Output_InitialisePolyList();
        Draw_DrawScene(false);
        Output_DumpScreen();
    }

    Output_FadeReset();

    if (level_num == g_GameFlow.last_level_num) {
        g_GameInfo.bonus_flag = GBF_NGPLUS;
    } else {
        CreateStartInfo(level_num + 1);
        ModifyStartInfo(level_num + 1);
    }

    g_GameInfo.start[g_CurrentLevel].flags.available = 0;
    Screen_ApplyResolution();
}