
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

    CRYPTO_PADDING_BLOCK_SIZE = 16,

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

typedef struct packed {byte _[CRYPTO_STREAM_CODER_SIZE];} CryptoStreamCoder;
typedef struct packed {byte _[CRYPTO_STREAM_HEADER_SIZE];} CryptoStreamHeader;

typedef struct packed {
    byte tag;
    byte mac[CRYPTO_STREAM_MAC_SIZE];
    byte data[];
} CryptoStreamEncryptedChunkBundle;

void cryptoStreamCreateEncoder(CryptoStreamCoder* const coder, CryptoStreamHeader* const header, const CryptoGenericKey* const key);
bool cryptoStreamCreateDecoder(CryptoStreamCoder* const coder, const CryptoStreamHeader* const header, const CryptoGenericKey* const key);
void cryptoStreamEncrypt(CryptoStreamCoder* const coder, CryptoStreamEncryptedChunkBundle* const bundle, const int dataSize);
bool cryptoStreamDecrypt(CryptoStreamCoder* const coder, CryptoStreamEncryptedChunkBundle* const bundle, const int dataSize);

// utils

void cryptoRandomBytes(byte* const buffer, const int size);
void cryptoZeroOutMemory(void* const memory, const int size);
bool cryptoNonceIncrementOverflowChecked(byte* const nonce, const int size); // little-endian! returns true on overflow

// encoding (url/path safe)

int cryptoBase64ResultSize(const int binarySize); // with trailing null byte
void cryptoBase64Encode(const byte* const binary, const int binarySize, char* const string, const int stringSize); // stringSize with trailing null byte
int cryptoBase64Decode(const char* const string, const int stringSize, byte* const binary, const int binarySize); // stringSize with trailing null byte, returns actual decoded size or -1 on failure

// padding

int cryptoPaddingAdd(byte* const padded, const int size); // size of padded is no more than size + padding_block_size and not less than size, returns the actual result size
int cryptoPaddingRemovedSize(const byte* const padded, const int size); // size = sizeof(padded), returns the actual original size or -1 on failure

// generic hash

typedef struct packed {byte _[CRYPTO_HASH_STATE_SIZE];} CryptoHashState;

void cryptoHash(
    CryptoHashState* nullable const state,
    const byte* nullable const data,
    const int dataSize,
    byte* nullable const output,
    const int hashSize
); // (state, data, output): single-part message - null, nonnull, nonnull; multipart init - nonnull, null, null; multipart step - nonnull, nonnull, null; multipart quit - nonnull, null, nonnull
