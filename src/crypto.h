
#pragma once

#include "defs.h"

typedef enum {
    CRYPTO_SIGN_PUBLIC_KEY_SIZE = 32,
    CRYPTO_SIGN_SECRET_KEY_SIZE = 64,
    CRYPTO_SIGNATURE_SIZE = 64,
    CRYPTO_ENCRYPT_PUBLIC_SECRET_KEY_SIZE = 32, // public* and secret* key sizes, seal(De)Encrypt size too
    CRYPTO_SEAL_SIZE = 48
} _CryptoConstant;

void cryptoInit(void);
bool cryptoInitialized(void);
const byte* cryptoMasterSignPublicKey(void); // sign*
const byte* cryptoMasterSessionSealPublicKey(void); // encrypt*
void cryptoMasterSign(const byte* const message, const int size, byte* const signature); // sizeof(signature) = signatureSize
bool cryptoCheckMasterSigned(const byte* const signedMessage, const int size); // size = signatureSize + sizeof(message)
void cryptoSeal(const byte* const message, const int size, byte* const sealedMessage, const byte* const sealPublicKey); // sizeof(sealedMessage) = sealSize + size
bool cryptoMasterSessionUnseal(const byte* const sealedMessage, const int size, byte* const unsealedMessage); // size = sealSize + sizeof(message)
void cryptoQuit(void);
