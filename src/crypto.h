
#pragma once

#include "defs.h"

extern const int
    CRYPTO_SIGN_PUBLIC_KEY_SIZE,
    CRYPTO_SIGN_SECRET_KEY_SIZE,
    CRYPTO_SIGNATURE_SIZE,
    CRYPTO_ENCRYPT_PUBLIC_KEY_SIZE,
    CRYPTO_ENCRYPT_SECRET_KEY_SIZE,
    CRYPTO_SEAL_SIZE;

void cryptoInit(void);
bool cryptoInitialized(void);
void cryptoMasterSign(const byte* const message, const int size, byte* const signedMessage); // sizeof(signedMessage) = signatureSize + size
bool cryptoCheckMasterSigned(const byte* const signedMessage, const int size); // size = signatureSize + sizeof(message)
void cryptoSeal(const byte* const message, const int size, byte* const sealedMessage, const byte* const sealPublicKey); // sizeof(sealedMessage) = sealSize + size
bool cryptoMasterUnseal(const byte* const sealedMessage, const int size, byte* const unsealedMessage); // size = sealSize + sizeof(message)
void cryptoQuit(void);
