#ifndef T1M_GAME_GAME_H
#define T1M_GAME_GAME_H

#include "global/types.h"

#include <stdint.h>

int32_t StartGame(int32_t level_num, GAMEFLOW_LEVEL_TYPE level_type);
int32_t StopGame();
int32_t GameLoop(int32_t demo_mode);
void GetSavedGamesList(REQUEST_INFO *req);
int32_t LevelCompleteSequence(int32_t level_num);
void SeedRandomControl(int32_t seed);
void SeedRandomDraw(int32_t seed);
int32_t GetRandomControl();
int32_t GetRandomDraw();
void LevelStats(int32_t level_num);
int32_t S_LoadGame(SAVEGAME_INFO *save, int32_t slot);
int32_t S_FrontEndCheck();
int32_t S_SaveGame(SAVEGAME_INFO *save, int32_t slot);

void T1MInjectGameGame();

#endif
