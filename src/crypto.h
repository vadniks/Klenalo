
#pragma once

#include "defs.h"

enum : int {
    CRYPTO_GENERIC_KEY_SIZE = 32,

    CRYPTO_SIGN_SECRET_KEY_SIZE = 64,
    CRYPTO_SIGNATURE_SIZE = 64,

    CRYPTO_SEAL_SIZE = 48,

    CRYPTO_SINGLE_CRYPT_NONCE_SIZE = 24,
    CRYPTO_SINGLE_CRYPT_MAC_SIZE = 16,

    CRYPTO_STREAM_HEADER_SIZE = 24,
    CRYPTO_STREAM_CODER_SIZE = 52,
    CRYPTO_STREAM_MAC_SIZE = 16,

    CRYPTO_HASH_STATE_SIZE = 384,
    CRYPTO_HASH_SMALL_SIZE = 32,
    CRYPTO_HASH_LARGE_SIZE = 64,
};

// don't try to access non-data fields in the more-than-one-field structures from the outside of this module -
// the actual memory layout isn't necessarily the same as the implementation's documentation doesn't fully describe these details

typedef struct packed {byte _[CRYPTO_GENERIC_KEY_SIZE];} CryptoGenericKey;

//

void cryptoInit(void);
void cryptoQuit(void);

// signature

typedef struct packed {byte _[CRYPTO_SIGN_SECRET_KEY_SIZE];} CryptoSignSecretKey;

typedef struct packed {
    byte signature[CRYPTO_SIGNATURE_SIZE];
    byte data[];
} CryptoSignedBundle;

void cryptoMakeSignKeypair(CryptoGenericKey* const publicKey, CryptoSignSecretKey* const secretKey);
void cryptoSign(CryptoSignedBundle* const bundle, const int dataSize, const CryptoSignSecretKey* const secretKey);
bool cryptoSignVerify(CryptoSignedBundle* const bundle, const int dataSize, const CryptoGenericKey* const publicKey);

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

void cryptoSingleEncrypt(CryptoSingleEncryptedBundle* const bundle, const int dataSize, const CryptoGenericKey* const key);
bool cryptoSingleDecrypt(CryptoSingleEncryptedBundle* const bundle, const int dataSize, const CryptoGenericKey* const key);

// stream encryption

//typedef struct packed {byte _[CRYPTO_STREAM_HEADER_SIZE];} CryptoStreamCoderHeader;
//typedef struct packed {byte _[CRYPTO_STREAM_CODER_SIZE];} CryptoStreamCoder;
//
//typedef struct packed {
//    byte mac[CRYPTO_STREAM_MAC_SIZE];
//    byte tag;
//    byte data[];
//} CryptoStreamEncryptedChunkBundle;

// utils

void cryptoRandomBytes(byte* const buffer, const int size);
void cryptoZeroOutMemory(void* const memory, const int size);
bool cryptoNonceIncrementOverflowChecked(byte* const nonce, const int size); // little-endian! returns true on overflow

// padding

// generic hash

typedef struct packed {byte _[CRYPTO_HASH_STATE_SIZE];} CryptoHashState;

void cryptoHash(
    CryptoHashState* nullable const state,
    const byte* nullable const data,
    const int dataSize,
    byte* nullable const output,
    const int hashSize
); // (state, data, output): single-part message - null, nonnull, nonnull; multipart init - nonnull, null, null; multipart step - nonnull, nonnull, null; multipart quit - nonnull, null, nonnull

// password hash
