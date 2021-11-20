#ifndef T1M_GAME_GAMEFLOW_H
#define T1M_GAME_GAMEFLOW_H

#include "global/types.h"

#include <stdint.h>

GAMEFLOW_OPTION
GF_InterpretSequence(int32_t level_num, GAMEFLOW_LEVEL_TYPE level_type);
bool GF_LoadScriptFile(const char *file_name);

#endif
