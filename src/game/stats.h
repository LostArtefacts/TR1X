#pragma once

#include "global/types.h"

#include <stdint.h>

void Stats_Init();
void Stats_CalculateStats(int32_t uninit_item_count, FLOOR_INFO **floor_array);
int32_t Stats_GetPickups();
int32_t Stats_GetKillables();
int32_t Stats_GetSecrets();
void Stats_LevelEnd(int32_t level_num);