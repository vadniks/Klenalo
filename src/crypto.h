
#pragma once

#include "defs.h"

enum : int {
    CRYPTO_SIGN_SECRET_KEY_SIZE = 64,
    CRYPTO_SIGNATURE_SIZE = 64,
    CRYPTO_GENERIC_KEY_SIZE = 32, // seal(De)Encrypt size, exchange public/secret key size, stream(De)Encrypt, single(De)Encrypt
    CRYPTO_SEAL_SIZE = 48,
    CRYPTO_STREAM_CODER_SIZE = 52,
    CRYPTO_STREAM_HEADER_SIZE = 24,
    CRYPTO_STREAM_AUTH_TAG_SIZE = 17,
    CRYPTO_PADDING_BLOCK_SIZE = 16,
    CRYPTO_SINGLE_CRYPT_AUTH_TAG_SIZE = 16,
    CRYPTO_SINGLE_CRYPT_NONCE_SIZE = 24
};

typedef struct packed {byte _[CRYPTO_GENERIC_KEY_SIZE];} CryptoGenericKey;
typedef struct packed {byte _[CRYPTO_SIGN_SECRET_KEY_SIZE];} CryptoSignSecretKey;

// don't try to access non-message fields in the more-than-one-field structs from the outside of this module -
// the actual memory layout isn't necessarily the same as the implementation's documentation doesn't fully describe these details

typedef struct packed {
    byte signature[CRYPTO_SIGNATURE_SIZE];
    byte message[];
} CryptoSigned;

typedef struct packed {
    byte seal[CRYPTO_SEAL_SIZE];
    byte message[];
} CryptoSealed;

typedef struct packed {const byte _[CRYPTO_STREAM_CODER_SIZE];} CryptoStreamCoder;
typedef struct packed {byte _[CRYPTO_STREAM_HEADER_SIZE];} CryptoStreamHeader;

typedef struct packed {
    byte authTag[CRYPTO_STREAM_AUTH_TAG_SIZE];
    byte message[];
} CryptoStreamEncryptedChunk;

typedef struct packed {
    byte nonce[CRYPTO_SINGLE_CRYPT_NONCE_SIZE];
    byte authTag[CRYPTO_SINGLE_CRYPT_AUTH_TAG_SIZE];
    byte message[];
} CryptoSingleEncrypted;

// TODO: create a minimal working prototype first, without "hard" things and then in each iteration add those things

// TODO: create a separate keyStore module

void cryptoInit(void);
bool cryptoInitialized(void);

void cryptoSign(const CryptoSignSecretKey* const secretKey, CryptoSigned* const xSigned, const int messageSize);
bool cryptoCheckSigned(const CryptoGenericKey* const publicKey, const CryptoSigned* const xSigned, const int messageSize);

void cryptoSeal(const CryptoGenericKey* const publicKey, CryptoSealed* const sealed, const int messageSize);
bool cryptoUnseal(const CryptoGenericKey* const publicKey, const CryptoGenericKey* const secretKey, CryptoSealed* const sealed, const int messageSize /*TODO: replace with (allocated) structSize*/);

void cryptoMakeStreamCoderForEncryption(const CryptoGenericKey* const key, CryptoStreamCoder* const coder, CryptoStreamHeader* const header);
bool cryptoMakeStreamCoderForDecryption(const CryptoGenericKey* const key, const CryptoStreamHeader* const header, CryptoStreamCoder* const coder);
void cryptoStreamEncrypt(CryptoStreamCoder* const coder, CryptoStreamEncryptedChunk* const chunk, const int messageSize);
bool cryptoStreamDecrypt(CryptoStreamCoder* const coder, CryptoStreamEncryptedChunk* const chunk, const int messageSize /*TODO: replace with (allocated) structSize*/);

void cryptoZeroOutMemory(void* const memory, const int size);

int cryptoAddPadding(byte* const message, const int size); // size = size of the actual message but not the whole buffer which size is assumed to be size + padding_block_size, accepts the editable buffer containing the original message, padding will be added to that buffer, returns the actual padded message size
int cryptoRemovePadding(byte* const message, const int size); // size = sizeof(message), message is padded, returns the original message's size or zero (false) on failure

void cryptoSingleEncrypt(CryptoGenericKey* const key, CryptoSingleEncrypted* const encrypted, const int messageSize, const bool makeNonce);
bool cryptoSingleDecrypt(CryptoGenericKey* const key, CryptoSingleEncrypted* const encrypted, const int messageSize /*TODO: replace with (allocated) structSize*/);

// TODO: singleDeterministic*crypt nonceIncrementOverflowChecked genericHashSingle genericHashMultiple passwordHash randomFill

void cryptoQuit(void);
