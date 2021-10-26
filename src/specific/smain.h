#ifndef T1M_SPECIFIC_SMAIN_H
#define T1M_SPECIFIC_SMAIN_H

#include <stdint.h>

void TerminateGame(int exit_code);
void ShowFatalError(const char *message);
void WinSpinMessageLoop();

void T1MInjectSpecificSMain();

#endif
