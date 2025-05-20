
#pragma once

#include "defs.h"

enum : int {
    CRYPTO_GENERIC_KEY_SIZE = 32,

    CRYPTO_SEAL_SIZE = 48,

    CRYPTO_SINGLE_CRYPT_NONCE_SIZE = 24,
    CRYPTO_SINGLE_CRYPT_MAC_SIZE = 16
};

typedef struct packed {byte _[CRYPTO_GENERIC_KEY_SIZE];} CryptoGenericKey;

//

void cryptoInit(void);
void cryptoQuit(void);

// public anonymous encryption (sealing)

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

// single message encryption

typedef struct packed {
    byte nonce[CRYPTO_SINGLE_CRYPT_NONCE_SIZE];
    byte mac[CRYPTO_SINGLE_CRYPT_MAC_SIZE];
    byte data[];
} CryptoSingleEncryptedBundle;

void cryptoRandomBytes(byte* const buffer, const int size);
void cryptoSingleEncrypt(CryptoSingleEncryptedBundle* const bundle, const int dataSize, const CryptoGenericKey* const key);
bool cryptoSingleDecrypt(CryptoSingleEncryptedBundle* const bundle, const int dataSize, const CryptoGenericKey* const key);
