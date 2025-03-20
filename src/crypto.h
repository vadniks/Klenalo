
#pragma once

#include "defs.h"

extern const int CRYPTO_SIGN_PUBLIC_KEY_SIZE, CRYPTO_SIGN_SECRET_KEY_SIZE, CRYPTO_SIGNATURE_SIZE;

void cryptoInit(void);
bool cryptoInitialized(void);
void cryptoMasterSign(const byte* const message, const int size, byte* const signedMessage); // sizeof(signedMessage) = signatureSize + size
bool cryptoCheckMasterSigned(const byte* const signedMessage, const int size); // size = signatureSize + sizeof(message)
void cryptoMasterSeal(const byte* const message, const int size, byte* const sealedMessage);
bool cryptoMasterUnseal(const byte* const sealedMessage, const int size, byte* const unsealedMessage);
void cryptoQuit(void);
