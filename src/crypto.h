
#pragma once

#include "defs.h"

enum : int {
    CRYPTO_GENERIC_KEY_SIZE = 32,
    CRYPTO_SEAL_SIZE = 48
};

typedef struct packed {byte _[CRYPTO_GENERIC_KEY_SIZE];} CryptoGenericKey;

void cryptoInit(void);
void cryptoQuit(void);

typedef struct packed {
    byte seal[CRYPTO_SEAL_SIZE];
    byte data[];
} CryptoPublicEncryptedBundle;

void cryptoMakeKeypair(CryptoGenericKey* const publicKey, CryptoGenericKey* const secretKey);
void cryptoPublicEncrypt(CryptoPublicEncryptedBundle* const bundle, const int dataSize, const CryptoGenericKey* const publicKey);
bool cryptoPublicDecrypt(
    CryptoPublicEncryptedBundle* const bundle,
    const int dataSize,
    const CryptoGenericKey* const publicKey,
    const CryptoGenericKey* const secretKey
);
