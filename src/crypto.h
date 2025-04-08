
#pragma once

#include "defs.h"

enum : int {
    CRYPTO_SIGN_PUBLIC_KEY_SIZE = 32,
    CRYPTO_SIGN_SECRET_KEY_SIZE = 64,
    CRYPTO_SIGNATURE_SIZE = 64,
    CRYPTO_ENCRYPT_PUBLIC_SECRET_KEY_SIZE = 32, // public* and secret* key sizes, seal(De)Encrypt size too
    CRYPTO_SEAL_SIZE = 48,
    CRYPTO_STREAM_CODER_SIZE = 52,
    CRYPTO_STREAM_CODER_HEADER_SIZE = 24,
    CRYPTO_STREAM_SERVICE_BYTES_SIZE = 17
};

typedef struct packed {
    const byte _[CRYPTO_STREAM_CODER_SIZE];
} StreamCoder;

void cryptoInit(void);
bool cryptoInitialized(void);
const byte* cryptoMasterSignPublicKey(void); // sign*
const byte* cryptoMasterSessionSealPublicKey(void); // encrypt*
void cryptoMasterSign(const byte* const message, const int size, byte* const signature); // sizeof(signature) = signatureSize
bool cryptoCheckMasterSigned(const byte* const message, const int size, const byte* const signature); // size = sizeof(message)
void cryptoSeal(const byte* const message, const int size, byte* const sealedMessage, const byte* const sealPublicKey); // sizeof(sealedMessage) = sealSize + size
bool cryptoMasterSessionUnseal(const byte* const sealedMessage, const int size, byte* const unsealedMessage); // size = sealSize + sizeof(message)
[[deprecated]] void cryptoMakeKeysForExchange(byte* const publicKey, byte* const secretKey); // buffers, sizeof(*Key) = encrypt_*_key_size
[[deprecated]] bool cryptoExchangeKeys(
    const bool initiator,
    byte* const receiveKey,
    byte* const sendKey,
    const byte* const ownPublicKey,
    const byte* const ownSecretKey,
    const byte* const foreignPublicKey
); // buffers and refs, sizeof(*Key) = encrypt_*_key_size
void cryptoMakeStreamCoderForEncryption(const byte* const key, StreamCoder* const coder, byte* const header); // takes pointers to buffers in which to write what's been made
bool cryptoMakeStreamCoderForDecryption(const byte* const key, const byte* const header, StreamCoder* const coder);
void cryptoStreamEncrypt(StreamCoder* const coder, const byte* const message, const int size, byte* const encrypted); // sizeof(encrypted) = size + stream_service_bytes_size
bool cryptoStreamDecrypt(StreamCoder* const coder, const byte* const encrypted, const int size, byte* const message);
// TODO: padding
void cryptoQuit(void);
