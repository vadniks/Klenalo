
#pragma once

#include "defs.h"

extern const int CRYPTO_PUBLIC_KEY_SIZE, CRYPTO_SECRET_KEY_SIZE, CRYPTO_SIGNATURE_SIZE;

void cryptoInit(void);
bool cryptoInitialized(void);
void cryptoMasterSign(const byte* const message, const int size, byte* const signedMessage); // sizeof(signedMessage) = signatureSize + size
bool cryptoCheckMasterSigned(const byte* const signedMessage, const int size); // size = signatureSize + sizeof(message)
void cryptoQuit(void);
