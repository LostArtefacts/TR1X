#include "game/inv.h"

#include "3dsystem/3d_gen.h"
#include "game/items.h"
#include "game/overlay.h"
#include "game/text.h"
#include "global/const.h"
#include "global/types.h"
#include "global/vars.h"
#include "specific/s_frontend.h"

#include <stdint.h>
#include <stdio.h>

TEXTSTRING *InvItemText[2] = { NULL, NULL };
TEXTSTRING *InvRingText = NULL;

static int16_t InvColours[IC_NUMBER_OF] = { 0 };
static TEXTSTRING *InvDownArrow1 = NULL;
static TEXTSTRING *InvDownArrow2 = NULL;
static TEXTSTRING *InvUpArrow1 = NULL;
static TEXTSTRING *InvUpArrow2 = NULL;

void InitColours()
{
    InvColours[IC_BLACK] = S_Colour(0, 0, 0);
    InvColours[IC_GREY] = S_Colour(64, 64, 64);
    InvColours[IC_WHITE] = S_Colour(255, 255, 255);
    InvColours[IC_RED] = S_Colour(255, 0, 0);
    InvColours[IC_ORANGE] = S_Colour(255, 128, 0);
    InvColours[IC_YELLOW] = S_Colour(255, 255, 0);
    InvColours[IC_DARKGREEN] = S_Colour(0, 128, 0);
    InvColours[IC_GREEN] = S_Colour(0, 255, 0);
    InvColours[IC_CYAN] = S_Colour(0, 255, 255);
    InvColours[IC_BLUE] = S_Colour(0, 0, 255);
    InvColours[IC_MAGENTA] = S_Colour(255, 0, 255);
}

void RingIsOpen(RING_INFO *ring)
{
    if (InvMode == INV_TITLE_MODE) {
        return;
    }

    if (!InvRingText) {
        switch (ring->type) {
        case RT_MAIN:
            InvRingText = Text_Create(0, 26, GF.strings[GS_HEADING_INVENTORY]);
            break;

        case RT_OPTION:
            if (InvMode == INV_DEATH_MODE) {
                InvRingText =
                    Text_Create(0, 26, GF.strings[GS_HEADING_GAME_OVER]);
            } else {
                InvRingText = Text_Create(0, 26, GF.strings[GS_HEADING_OPTION]);
            }
            break;

        case RT_KEYS:
            InvRingText = Text_Create(0, 26, GF.strings[GS_HEADING_ITEMS]);
            break;
        }

        Text_CentreH(InvRingText, 1);
    }

    if (InvMode == INV_KEYS_MODE || InvMode == INV_DEATH_MODE) {
        return;
    }

    if (!InvUpArrow1) {
        if (ring->type == RT_OPTION
            || (ring->type == RT_MAIN && InvKeysObjects)) {
            InvUpArrow1 = Text_Create(20, 28, "[");
            InvUpArrow2 = Text_Create(-20, 28, "[");
            Text_AlignRight(InvUpArrow2, 1);
        }
    }

    if (!InvDownArrow1) {
        if (ring->type == RT_MAIN || ring->type == RT_KEYS) {
            InvDownArrow1 = Text_Create(20, -15, "]");
            InvDownArrow2 = Text_Create(-20, -15, "]");
            Text_AlignBottom(InvDownArrow1, 1);
            Text_AlignBottom(InvDownArrow2, 1);
            Text_AlignRight(InvDownArrow2, 1);
        }
    }
}

void RingIsNotOpen(RING_INFO *ring)
{
    if (!InvRingText) {
        return;
    }

    Text_Remove(InvRingText);
    InvRingText = NULL;

    if (InvUpArrow1) {
        Text_Remove(InvUpArrow1);
        Text_Remove(InvUpArrow2);
        InvUpArrow1 = NULL;
        InvUpArrow2 = NULL;
    }
    if (InvDownArrow1) {
        Text_Remove(InvDownArrow1);
        Text_Remove(InvDownArrow2);
        InvDownArrow1 = NULL;
        InvDownArrow2 = NULL;
    }
}

void RingNotActive(INVENTORY_ITEM *inv_item)
{
    if (!InvItemText[IT_NAME]) {
        switch (inv_item->object_number) {
        case O_PUZZLE_OPTION1:
            InvItemText[IT_NAME] =
                Text_Create(0, -16, GF.levels[CurrentLevel].puzzle1);
            break;

        case O_PUZZLE_OPTION2:
            InvItemText[IT_NAME] =
                Text_Create(0, -16, GF.levels[CurrentLevel].puzzle2);
            break;

        case O_PUZZLE_OPTION3:
            InvItemText[IT_NAME] =
                Text_Create(0, -16, GF.levels[CurrentLevel].puzzle3);
            break;

        case O_PUZZLE_OPTION4:
            InvItemText[IT_NAME] =
                Text_Create(0, -16, GF.levels[CurrentLevel].puzzle4);
            break;

        case O_KEY_OPTION1:
            InvItemText[IT_NAME] =
                Text_Create(0, -16, GF.levels[CurrentLevel].key1);
            break;

        case O_KEY_OPTION2:
            InvItemText[IT_NAME] =
                Text_Create(0, -16, GF.levels[CurrentLevel].key2);
            break;

        case O_KEY_OPTION3:
            InvItemText[IT_NAME] =
                Text_Create(0, -16, GF.levels[CurrentLevel].key3);
            break;

        case O_KEY_OPTION4:
            InvItemText[IT_NAME] =
                Text_Create(0, -16, GF.levels[CurrentLevel].key4);
            break;

        case O_PICKUP_OPTION1:
            InvItemText[IT_NAME] =
                Text_Create(0, -16, GF.levels[CurrentLevel].pickup1);
            break;

        case O_PICKUP_OPTION2:
            InvItemText[IT_NAME] =
                Text_Create(0, -16, GF.levels[CurrentLevel].pickup2);
            break;

        case O_PASSPORT_OPTION:
            break;

        default:
            InvItemText[IT_NAME] = Text_Create(0, -16, inv_item->string);
            break;
        }

        if (InvItemText[IT_NAME]) {
            Text_AlignBottom(InvItemText[IT_NAME], 1);
            Text_CentreH(InvItemText[IT_NAME], 1);
        }
    }

    char temp_text[64];
    int32_t qty = Inv_RequestItem(inv_item->object_number);

    switch (inv_item->object_number) {
    case O_SHOTGUN_OPTION:
        if (!InvItemText[IT_QTY] && !(SaveGame.bonus_flag & GBF_NGPLUS)) {
            sprintf(temp_text, "%5d A", Lara.shotgun.ammo / SHOTGUN_AMMO_CLIP);
            Overlay_MakeAmmoString(temp_text);
            InvItemText[IT_QTY] = Text_Create(64, -56, temp_text);
            Text_AlignBottom(InvItemText[IT_QTY], 1);
            Text_CentreH(InvItemText[IT_QTY], 1);
        }
        break;

    case O_MAGNUM_OPTION:
        if (!InvItemText[IT_QTY] && !(SaveGame.bonus_flag & GBF_NGPLUS)) {
            sprintf(temp_text, "%5d B", Lara.magnums.ammo);
            Overlay_MakeAmmoString(temp_text);
            InvItemText[IT_QTY] = Text_Create(64, -56, temp_text);
            Text_AlignBottom(InvItemText[IT_QTY], 1);
            Text_CentreH(InvItemText[IT_QTY], 1);
        }
        break;

    case O_UZI_OPTION:
        if (!InvItemText[IT_QTY] && !(SaveGame.bonus_flag & GBF_NGPLUS)) {
            sprintf(temp_text, "%5d C", Lara.uzis.ammo);
            Overlay_MakeAmmoString(temp_text);
            InvItemText[IT_QTY] = Text_Create(64, -56, temp_text);
            Text_AlignBottom(InvItemText[IT_QTY], 1);
            Text_CentreH(InvItemText[IT_QTY], 1);
        }
        break;

    case O_SG_AMMO_OPTION:
        if (!InvItemText[IT_QTY]) {
            sprintf(temp_text, "%d", qty * NUM_SG_SHELLS);
            Overlay_MakeAmmoString(temp_text);
            InvItemText[IT_QTY] = Text_Create(64, -56, temp_text);
            Text_AlignBottom(InvItemText[IT_QTY], 1);
            Text_CentreH(InvItemText[IT_QTY], 1);
        }
        break;

    case O_MAG_AMMO_OPTION:
        if (!InvItemText[IT_QTY]) {
            sprintf(temp_text, "%d", Inv_RequestItem(O_MAG_AMMO_OPTION) * 2);
            Overlay_MakeAmmoString(temp_text);
            InvItemText[IT_QTY] = Text_Create(64, -56, temp_text);
            Text_AlignBottom(InvItemText[IT_QTY], 1);
            Text_CentreH(InvItemText[IT_QTY], 1);
        }
        break;

    case O_UZI_AMMO_OPTION:
        if (!InvItemText[IT_QTY]) {
            sprintf(temp_text, "%d", Inv_RequestItem(O_UZI_AMMO_OPTION) * 2);
            Overlay_MakeAmmoString(temp_text);
            InvItemText[IT_QTY] = Text_Create(64, -56, temp_text);
            Text_AlignBottom(InvItemText[IT_QTY], 1);
            Text_CentreH(InvItemText[IT_QTY], 1);
        }
        break;

    case O_MEDI_OPTION:
        HealthBarTimer = 40;
        Overlay_DrawHealthBar();
        if (!InvItemText[IT_QTY] && qty > 1) {
            sprintf(temp_text, "%d", qty);
            Overlay_MakeAmmoString(temp_text);
            InvItemText[IT_QTY] = Text_Create(64, -56, temp_text);
            Text_AlignBottom(InvItemText[IT_QTY], 1);
            Text_CentreH(InvItemText[IT_QTY], 1);
        }
        break;

    case O_BIGMEDI_OPTION:
        HealthBarTimer = 40;
        Overlay_DrawHealthBar();
        if (!InvItemText[IT_QTY] && qty > 1) {
            sprintf(temp_text, "%d", qty);
            Overlay_MakeAmmoString(temp_text);
            InvItemText[IT_QTY] = Text_Create(64, -56, temp_text);
            Text_AlignBottom(InvItemText[IT_QTY], 1);
            Text_CentreH(InvItemText[IT_QTY], 1);
        }
        break;

    case O_KEY_OPTION1:
    case O_KEY_OPTION2:
    case O_KEY_OPTION3:
    case O_KEY_OPTION4:
    case O_LEADBAR_OPTION:
    case O_PICKUP_OPTION1:
    case O_PICKUP_OPTION2:
    case O_PUZZLE_OPTION1:
    case O_PUZZLE_OPTION2:
    case O_PUZZLE_OPTION3:
    case O_PUZZLE_OPTION4:
    case O_SCION_OPTION:
        if (!InvItemText[IT_QTY] && qty > 1) {
            sprintf(temp_text, "%d", qty);
            Overlay_MakeAmmoString(temp_text);
            InvItemText[IT_QTY] = Text_Create(64, -56, temp_text);
            Text_AlignBottom(InvItemText[IT_QTY], 1);
            Text_CentreH(InvItemText[IT_QTY], 1);
        }
        break;
    }
}

void RingActive()
{
    if (InvItemText[IT_NAME]) {
        Text_Remove(InvItemText[IT_NAME]);
        InvItemText[IT_NAME] = NULL;
    }
    if (InvItemText[IT_QTY]) {
        Text_Remove(InvItemText[IT_QTY]);
        InvItemText[IT_QTY] = NULL;
    }
}

int32_t Inv_AddItem(int32_t item_num)
{
    int32_t item_num_option = Inv_GetItemOption(item_num);

    for (int i = 0; i < InvMainObjects; i++) {
        INVENTORY_ITEM *inv_item = InvMainList[i];
        if (inv_item->object_number == item_num_option) {
            InvMainQtys[i]++;
            return 1;
        }
    }

    for (int i = 0; i < InvKeysObjects; i++) {
        INVENTORY_ITEM *inv_item = InvKeysList[i];
        if (inv_item->object_number == item_num_option) {
            InvKeysQtys[i]++;
            return 1;
        }
    }

    switch (item_num) {
    case O_GUN_ITEM:
    case O_GUN_OPTION:
        Inv_InsertItem(&InvItemPistols);
        return 1;

    case O_SHOTGUN_ITEM:
    case O_SHOTGUN_OPTION:
        for (int i = Inv_RequestItem(O_SG_AMMO_ITEM); i > 0; i--) {
            Inv_RemoveItem(O_SG_AMMO_ITEM);
            Lara.shotgun.ammo += SHOTGUN_AMMO_QTY;
        }
        Lara.shotgun.ammo += SHOTGUN_AMMO_QTY;
        Inv_InsertItem(&InvItemShotgun);
        GlobalItemReplace(O_SHOTGUN_ITEM, O_SG_AMMO_ITEM);
        return 0;

    case O_MAGNUM_ITEM:
    case O_MAGNUM_OPTION:
        for (int i = Inv_RequestItem(O_MAG_AMMO_ITEM); i > 0; i--) {
            Inv_RemoveItem(O_MAG_AMMO_ITEM);
            Lara.magnums.ammo += MAGNUM_AMMO_QTY;
        }
        Lara.magnums.ammo += MAGNUM_AMMO_QTY;
        Inv_InsertItem(&InvItemMagnum);
        GlobalItemReplace(O_MAGNUM_ITEM, O_MAG_AMMO_ITEM);
        return 0;

    case O_UZI_ITEM:
    case O_UZI_OPTION:
        for (int i = Inv_RequestItem(O_UZI_AMMO_ITEM); i > 0; i--) {
            Inv_RemoveItem(O_UZI_AMMO_ITEM);
            Lara.uzis.ammo += UZI_AMMO_QTY;
        }
        Lara.uzis.ammo += UZI_AMMO_QTY;
        Inv_InsertItem(&InvItemUzi);
        GlobalItemReplace(O_UZI_ITEM, O_UZI_AMMO_ITEM);
        return 0;

    case O_SG_AMMO_ITEM:
    case O_SG_AMMO_OPTION:
        if (Inv_RequestItem(O_SHOTGUN_ITEM)) {
            Lara.shotgun.ammo += SHOTGUN_AMMO_QTY;
        } else {
            Inv_InsertItem(&InvItemShotgunAmmo);
        }
        return 0;

    case O_MAG_AMMO_ITEM:
    case O_MAG_AMMO_OPTION:
        if (Inv_RequestItem(O_MAGNUM_ITEM)) {
            Lara.magnums.ammo += MAGNUM_AMMO_QTY;
        } else {
            Inv_InsertItem(&InvItemMagnumAmmo);
        }
        return 0;

    case O_UZI_AMMO_ITEM:
    case O_UZI_AMMO_OPTION:
        if (Inv_RequestItem(O_UZI_ITEM)) {
            Lara.uzis.ammo += UZI_AMMO_QTY;
        } else {
            Inv_InsertItem(&InvItemUziAmmo);
        }
        return 0;

    case O_MEDI_ITEM:
    case O_MEDI_OPTION:
        Inv_InsertItem(&InvItemMedi);
        return 1;

    case O_BIGMEDI_ITEM:
    case O_BIGMEDI_OPTION:
        Inv_InsertItem(&InvItemBigMedi);
        return 1;

    case O_PUZZLE_ITEM1:
    case O_PUZZLE_OPTION1:
        Inv_InsertItem(&InvItemPuzzle1);
        return 1;

    case O_PUZZLE_ITEM2:
    case O_PUZZLE_OPTION2:
        Inv_InsertItem(&InvItemPuzzle2);
        return 1;

    case O_PUZZLE_ITEM3:
    case O_PUZZLE_OPTION3:
        Inv_InsertItem(&InvItemPuzzle3);
        return 1;

    case O_PUZZLE_ITEM4:
    case O_PUZZLE_OPTION4:
        Inv_InsertItem(&InvItemPuzzle4);
        return 1;

    case O_LEADBAR_ITEM:
    case O_LEADBAR_OPTION:
        Inv_InsertItem(&InvItemLeadBar);
        return 1;

    case O_KEY_ITEM1:
    case O_KEY_OPTION1:
        Inv_InsertItem(&InvItemKey1);
        return 1;

    case O_KEY_ITEM2:
    case O_KEY_OPTION2:
        Inv_InsertItem(&InvItemKey2);
        return 1;

    case O_KEY_ITEM3:
    case O_KEY_OPTION3:
        Inv_InsertItem(&InvItemKey3);
        return 1;

    case O_KEY_ITEM4:
    case O_KEY_OPTION4:
        Inv_InsertItem(&InvItemKey4);
        return 1;

    case O_PICKUP_ITEM1:
    case O_PICKUP_OPTION1:
        Inv_InsertItem(&InvItemPickup1);
        return 1;

    case O_PICKUP_ITEM2:
    case O_PICKUP_OPTION2:
        Inv_InsertItem(&InvItemPickup2);
        return 1;

    case O_SCION_ITEM:
    case O_SCION_ITEM2:
    case O_SCION_OPTION:
        Inv_InsertItem(&InvItemScion);
        return 1;
    }

    return 0;
}

void Inv_InsertItem(INVENTORY_ITEM *inv_item)
{
    int n;

    if (inv_item->inv_pos < 100) {
        for (n = 0; n < InvMainObjects; n++) {
            if (InvMainList[n]->inv_pos > inv_item->inv_pos) {
                break;
            }
        }

        if (n == InvMainObjects) {
            InvMainList[InvMainObjects] = inv_item;
            InvMainQtys[InvMainObjects] = 1;
            InvMainObjects++;
        } else {
            for (int i = InvMainObjects; i > n - 1; i--) {
                InvMainList[i + 1] = InvMainList[i];
                InvMainQtys[i + 1] = InvMainQtys[i];
            }
            InvMainList[n] = inv_item;
            InvMainQtys[n] = 1;
            InvMainObjects++;
        }
    } else {
        for (n = 0; n < InvKeysObjects; n++) {
            if (InvKeysList[n]->inv_pos > inv_item->inv_pos) {
                break;
            }
        }

        if (n == InvKeysObjects) {
            InvKeysList[InvKeysObjects] = inv_item;
            InvKeysQtys[InvKeysObjects] = 1;
            InvKeysObjects++;
        } else {
            for (int i = InvKeysObjects; i > n - 1; i--) {
                InvKeysList[i + 1] = InvKeysList[i];
                InvKeysQtys[i + 1] = InvKeysQtys[i];
            }
            InvKeysList[n] = inv_item;
            InvKeysQtys[n] = 1;
            InvKeysObjects++;
        }
    }
}

int32_t Inv_RequestItem(int item_num)
{
    int32_t item_num_option = Inv_GetItemOption(item_num);

    for (int i = 0; i < InvMainObjects; i++) {
        if (InvMainList[i]->object_number == item_num_option) {
            return InvMainQtys[i];
        }
    }

    for (int i = 0; i < InvKeysObjects; i++) {
        if (InvKeysList[i]->object_number == item_num_option) {
            return InvKeysQtys[i];
        }
    }

    return 0;
}

void Inv_RemoveAllItems()
{
    InvMainObjects = 1;
    InvMainCurrent = 0;

    InvKeysObjects = 0;
    InvKeysCurrent = 0;
}

int32_t Inv_RemoveItem(int32_t item_num)
{
    int32_t item_num_option = Inv_GetItemOption(item_num);

    for (int i = 0; i < InvMainObjects; i++) {
        if (InvMainList[i]->object_number == item_num_option) {
            InvMainQtys[i]--;
            if (InvMainQtys[i] > 0) {
                return 1;
            }
            InvMainObjects--;
            for (int j = i; j < InvMainObjects; j++) {
                InvMainList[j] = InvMainList[j + 1];
                InvMainQtys[j] = InvMainQtys[j + 1];
            }
        }
    }

    for (int i = 0; i < InvKeysObjects; i++) {
        if (InvKeysList[i]->object_number == item_num_option) {
            InvKeysQtys[i]--;
            if (InvKeysQtys[i] > 0) {
                return 1;
            }
            InvKeysObjects--;
            for (int j = i; j < InvKeysObjects; j++) {
                InvKeysList[j] = InvKeysList[j + 1];
                InvKeysQtys[j] = InvKeysQtys[j + 1];
            }
            return 1;
        }
    }

    for (int i = 0; i < InvOptionObjects; i++) {
        if (InvOptionList[i]->object_number == item_num_option) {
            InvOptionObjects--;
            for (int j = i; j < InvOptionObjects; j++) {
                InvOptionList[j] = InvOptionList[j + 1];
            }
            return 1;
        }
    }

    return 0;
}

int32_t Inv_GetItemOption(int32_t item_num)
{
    switch (item_num) {
    case O_GUN_ITEM:
    case O_GUN_OPTION:
        return O_GUN_OPTION;

    case O_SHOTGUN_ITEM:
    case O_SHOTGUN_OPTION:
        return O_SHOTGUN_OPTION;

    case O_MAGNUM_ITEM:
    case O_MAGNUM_OPTION:
        return O_MAGNUM_OPTION;

    case O_UZI_ITEM:
    case O_UZI_OPTION:
        return O_UZI_OPTION;

    case O_SG_AMMO_ITEM:
    case O_SG_AMMO_OPTION:
        return O_SG_AMMO_OPTION;

    case O_MAG_AMMO_ITEM:
    case O_MAG_AMMO_OPTION:
        return O_MAG_AMMO_OPTION;

    case O_UZI_AMMO_ITEM:
    case O_UZI_AMMO_OPTION:
        return O_UZI_AMMO_OPTION;

    case O_EXPLOSIVE_ITEM:
    case O_EXPLOSIVE_OPTION:
        return O_EXPLOSIVE_OPTION;

    case O_MEDI_ITEM:
    case O_MEDI_OPTION:
        return O_MEDI_OPTION;

    case O_BIGMEDI_ITEM:
    case O_BIGMEDI_OPTION:
        return O_BIGMEDI_OPTION;

    case O_PUZZLE_ITEM1:
    case O_PUZZLE_OPTION1:
        return O_PUZZLE_OPTION1;

    case O_PUZZLE_ITEM2:
    case O_PUZZLE_OPTION2:
        return O_PUZZLE_OPTION2;

    case O_PUZZLE_ITEM3:
    case O_PUZZLE_OPTION3:
        return O_PUZZLE_OPTION3;

    case O_PUZZLE_ITEM4:
    case O_PUZZLE_OPTION4:
        return O_PUZZLE_OPTION4;

    case O_LEADBAR_ITEM:
    case O_LEADBAR_OPTION:
        return O_LEADBAR_OPTION;

    case O_KEY_ITEM1:
    case O_KEY_OPTION1:
        return O_KEY_OPTION1;

    case O_KEY_ITEM2:
    case O_KEY_OPTION2:
        return O_KEY_OPTION2;

    case O_KEY_ITEM3:
    case O_KEY_OPTION3:
        return O_KEY_OPTION3;

    case O_KEY_ITEM4:
    case O_KEY_OPTION4:
        return O_KEY_OPTION4;

    case O_PICKUP_ITEM1:
    case O_PICKUP_OPTION1:
        return O_PICKUP_OPTION1;

    case O_PICKUP_ITEM2:
    case O_PICKUP_OPTION2:
        return O_PICKUP_OPTION2;

    case O_SCION_ITEM:
    case O_SCION_ITEM2:
    case O_SCION_OPTION:
        return O_SCION_OPTION;

    case O_DETAIL_OPTION:
    case O_SOUND_OPTION:
    case O_CONTROL_OPTION:
    case O_GAMMA_OPTION:
    case O_PASSPORT_OPTION:
    case O_MAP_OPTION:
    case O_PHOTO_OPTION:
        return item_num;
    }

    return -1;
}

void RemoveInventoryText()
{
    for (int i = 0; i < IT_NUMBER_OF; i++) {
        if (InvItemText[i]) {
            Text_Remove(InvItemText[i]);
            InvItemText[i] = NULL;
        }
    }
}

void Inv_RingInit(
    RING_INFO *ring, int16_t type, INVENTORY_ITEM **list, int16_t qty,
    int16_t current, IMOTION_INFO *imo)
{
    ring->type = type;
    ring->radius = 0;
    ring->list = list;
    ring->number_of_objects = qty;
    ring->current_object = current;
    ring->angle_adder = 0x10000 / qty;

    if (InvMode == INV_TITLE_MODE) {
        ring->camera_pitch = 1024;
    } else {
        ring->camera_pitch = 0;
    }
    ring->rotating = 0;
    ring->rot_count = 0;
    ring->target_object = 0;
    ring->rot_adder = 0;
    ring->rot_adder_l = 0;
    ring->rot_adder_r = 0;

    ring->imo = imo;

    ring->camera.x = 0;
    ring->camera.y = CAMERA_STARTHEIGHT;
    ring->camera.z = 896;
    ring->camera.x_rot = 0;
    ring->camera.y_rot = 0;
    ring->camera.z_rot = 0;

    Inv_RingMotionInit(ring, OPEN_FRAMES, RNG_OPENING, RNG_OPEN);
    Inv_RingMotionRadius(ring, RING_RADIUS);
    Inv_RingMotionCameraPos(ring, CAMERA_HEIGHT);
    Inv_RingMotionRotation(
        ring, OPEN_ROTATION,
        0xC000 - (ring->current_object * ring->angle_adder));

    ring->ringpos.x = 0;
    ring->ringpos.y = 0;
    ring->ringpos.z = 0;
    ring->ringpos.x_rot = 0;
    ring->ringpos.y_rot = imo->rotate_target - OPEN_ROTATION;
    ring->ringpos.z_rot = 0;

    ring->light.x = -1536;
    ring->light.y = 256;
    ring->light.z = 1024;
}

void Inv_RingGetView(RING_INFO *ring, PHD_3DPOS *viewer)
{
    PHD_ANGLE angles[2];

    phd_GetVectorAngles(
        -ring->camera.x, CAMERA_YOFFSET - ring->camera.y,
        ring->radius - ring->camera.z, angles);
    viewer->x = ring->camera.x;
    viewer->y = ring->camera.y;
    viewer->z = ring->camera.z;
    viewer->x_rot = angles[1] + ring->camera_pitch;
    viewer->y_rot = angles[0];
    viewer->z_rot = 0;
}

void Inv_RingLight(RING_INFO *ring)
{
    PHD_ANGLE angles[2];
    LsDivider = 0x6000;
    phd_GetVectorAngles(ring->light.x, ring->light.y, ring->light.z, angles);
    phd_RotateLight(angles[1], angles[0]);
}

void Inv_RingCalcAdders(RING_INFO *ring, int16_t rotation_duration)
{
    ring->angle_adder = 0x10000 / ring->number_of_objects;
    ring->rot_adder_l = ring->angle_adder / rotation_duration;
    ring->rot_adder_r = -ring->rot_adder_l;
}

void Inv_RingDoMotions(RING_INFO *ring)
{
    IMOTION_INFO *imo = ring->imo;

    if (imo->count) {
        ring->radius += imo->radius_rate;
        ring->camera.y += imo->camera_yrate;
        ring->ringpos.y_rot += imo->rotate_rate;
        ring->camera_pitch += imo->camera_pitch_rate;

        INVENTORY_ITEM *inv_item = ring->list[ring->current_object];
        inv_item->pt_xrot += imo->item_ptxrot_rate;
        inv_item->x_rot += imo->item_xrot_rate;
        inv_item->ytrans += imo->item_ytrans_rate;
        inv_item->ztrans += imo->item_ztrans_rate;

        imo->count--;
        if (!imo->count) {
            imo->status = imo->status_target;
            if (imo->radius_rate) {
                imo->radius_rate = 0;
                ring->radius = imo->radius_target;
            }
            if (imo->camera_yrate) {
                imo->camera_yrate = 0;
                ring->camera.y = imo->camera_ytarget;
            }
            if (imo->rotate_rate) {
                imo->rotate_rate = 0;
                ring->ringpos.y_rot = imo->rotate_target;
            }
            if (imo->item_ptxrot_rate) {
                imo->item_ptxrot_rate = 0;
                inv_item->pt_xrot = imo->item_ptxrot_target;
            }
            if (imo->item_xrot_rate) {
                imo->item_xrot_rate = 0;
                inv_item->x_rot = imo->item_xrot_target;
            }
            if (imo->item_ytrans_rate) {
                imo->item_ytrans_rate = 0;
                inv_item->ytrans = imo->item_ytrans_target;
            }
            if (imo->item_ztrans_rate) {
                imo->item_ztrans_rate = 0;
                inv_item->ztrans = imo->item_ztrans_target;
            }
            if (imo->camera_pitch_rate) {
                imo->camera_pitch_rate = 0;
                ring->camera_pitch = imo->camera_pitch_target;
            }
        }
    }

    if (ring->rotating) {
        ring->ringpos.y_rot += ring->rot_adder;
        ring->rot_count--;
        if (!ring->rot_count) {
            ring->current_object = ring->target_object;
            ring->ringpos.y_rot =
                0xC000 - (ring->current_object * ring->angle_adder);
            ring->rotating = 0;
        }
    }
}

void Inv_RingRotateLeft(RING_INFO *ring)
{
    ring->rotating = 1;
    ring->target_object = ring->current_object - 1;
    if (ring->target_object < 0) {
        ring->target_object = ring->number_of_objects - 1;
    }
    ring->rot_count = ROTATE_DURATION;
    ring->rot_adder = ring->rot_adder_l;
}

void Inv_RingRotateRight(RING_INFO *ring)
{
    ring->rotating = 1;
    ring->target_object = ring->current_object + 1;
    if (ring->target_object >= ring->number_of_objects) {
        ring->target_object = 0;
    }
    ring->rot_count = ROTATE_DURATION;
    ring->rot_adder = ring->rot_adder_r;
}

void Inv_RingMotionInit(
    RING_INFO *ring, int16_t frames, int16_t status, int16_t status_target)
{
    ring->imo->status_target = status_target;
    ring->imo->count = frames;
    ring->imo->status = status;
    ring->imo->radius_target = 0;
    ring->imo->radius_rate = 0;
    ring->imo->camera_ytarget = 0;
    ring->imo->camera_yrate = 0;
    ring->imo->camera_pitch_target = 0;
    ring->imo->camera_pitch_rate = 0;
    ring->imo->rotate_target = 0;
    ring->imo->rotate_rate = 0;
    ring->imo->item_ptxrot_target = 0;
    ring->imo->item_ptxrot_rate = 0;
    ring->imo->item_xrot_target = 0;
    ring->imo->item_xrot_rate = 0;
    ring->imo->item_ytrans_target = 0;
    ring->imo->item_ytrans_rate = 0;
    ring->imo->item_ztrans_target = 0;
    ring->imo->item_ztrans_rate = 0;
    ring->imo->misc = 0;
}

void Inv_RingMotionSetup(
    RING_INFO *ring, int16_t status, int16_t status_target, int16_t frames)
{
    IMOTION_INFO *imo = ring->imo;
    imo->count = frames;
    imo->status = status;
    imo->status_target = status_target;
    imo->radius_rate = 0;
    imo->camera_yrate = 0;
}

void Inv_RingMotionRadius(RING_INFO *ring, int16_t target)
{
    IMOTION_INFO *imo = ring->imo;
    imo->radius_target = target;
    imo->radius_rate = (target - ring->radius) / imo->count;
}

void Inv_RingMotionRotation(RING_INFO *ring, int16_t rotation, int16_t target)
{
    IMOTION_INFO *imo = ring->imo;
    imo->rotate_target = target;
    imo->rotate_rate = rotation / imo->count;
}

void Inv_RingMotionCameraPos(RING_INFO *ring, int16_t target)
{
    IMOTION_INFO *imo = ring->imo;
    imo->camera_ytarget = target;
    imo->camera_yrate = (target - ring->camera.y) / imo->count;
}

void Inv_RingMotionCameraPitch(RING_INFO *ring, int16_t target)
{
    IMOTION_INFO *imo = ring->imo;
    imo->camera_pitch_target = target;
    imo->camera_pitch_rate = target / imo->count;
}

void Inv_RingMotionItemSelect(RING_INFO *ring, INVENTORY_ITEM *inv_item)
{
    IMOTION_INFO *imo = ring->imo;
    imo->item_ptxrot_target = inv_item->pt_xrot_sel;
    imo->item_ptxrot_rate = inv_item->pt_xrot_sel / imo->count;
    imo->item_xrot_target = inv_item->x_rot_sel;
    imo->item_xrot_rate = inv_item->x_rot_sel / imo->count;
    imo->item_ytrans_target = inv_item->ytrans_sel;
    imo->item_ytrans_rate = inv_item->ytrans_sel / imo->count;
    imo->item_ztrans_target = inv_item->ztrans_sel;
    imo->item_ztrans_rate = inv_item->ztrans_sel / imo->count;
}

void Inv_RingMotionItemDeselect(RING_INFO *ring, INVENTORY_ITEM *inv_item)
{
    IMOTION_INFO *imo = ring->imo;
    imo->item_ptxrot_target = 0;
    imo->item_ptxrot_rate = -inv_item->pt_xrot_sel / imo->count;
    imo->item_xrot_target = 0;
    imo->item_xrot_rate = -inv_item->x_rot_sel / imo->count;
    imo->item_ytrans_target = 0;
    imo->item_ytrans_rate = -inv_item->ytrans_sel / imo->count;
    imo->item_ztrans_target = 0;
    imo->item_ztrans_rate = -inv_item->ztrans_sel / imo->count;
}
