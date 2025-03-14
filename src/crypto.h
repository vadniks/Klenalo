
#pragma once

#include "defs.h"

void cryptoInit(void);
bool cryptoInitialized(void);
void cryptoMasterSign(const byte* const message, const int size, byte* const buffer);
void cryptoQuit(void);
