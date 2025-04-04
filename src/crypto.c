
#include <sodium.h>
#include "lifecycle.h"
#include "crypto.h"

typedef unsigned long long xlong;

staticAssert(
    (CRYPTO_SIGN_PUBLIC_KEY_SIZE == crypto_sign_PUBLICKEYBYTES) &
    (CRYPTO_SIGN_SECRET_KEY_SIZE == crypto_sign_SECRETKEYBYTES) &
    (CRYPTO_SIGNATURE_SIZE == crypto_sign_BYTES) &
        (crypto_sign_BYTES == crypto_sign_SECRETKEYBYTES) &
    (CRYPTO_ENCRYPT_PUBLIC_SECRET_KEY_SIZE == crypto_box_PUBLICKEYBYTES) &
        (crypto_box_PUBLICKEYBYTES == crypto_box_SECRETKEYBYTES) &
        (crypto_box_PUBLICKEYBYTES == crypto_sign_PUBLICKEYBYTES) &
        (crypto_box_PUBLICKEYBYTES == crypto_sign_SECRETKEYBYTES / 2) &
        (crypto_box_PUBLICKEYBYTES == crypto_secretbox_KEYBYTES) &
        (crypto_box_PUBLICKEYBYTES == crypto_kx_PUBLICKEYBYTES) &
        (crypto_box_PUBLICKEYBYTES == crypto_kx_SECRETKEYBYTES) &
        (crypto_box_PUBLICKEYBYTES == crypto_kx_SESSIONKEYBYTES) &
        (crypto_box_PUBLICKEYBYTES == crypto_secretstream_xchacha20poly1305_KEYBYTES) &
    (CRYPTO_SEAL_SIZE == crypto_box_SEALBYTES)
);

// TODO: will be replaced with config file where users can set the keys themselves
static const byte gMasterSignPublicKey[CRYPTO_SIGN_PUBLIC_KEY_SIZE] = "\xa9\x10\x98\xdc\x68\xfb\x26\x29\x71\xbc\x23\x57\x4a\x7\xe7\xc3\x22\x44\x82\x91\xd8\xe4\x3\x88\x82\xad\xbe\x18\xc2\x4e\xef\x77";
static const byte gMasterSignSecretKey[CRYPTO_SIGN_SECRET_KEY_SIZE] = "\xde\x51\x0\xb3\xf3\x68\xf0\x93\x9c\x51\x0\x19\x86\x53\xc3\x99\xc9\xa7\xc2\x23\x9a\xa4\x46\x26\x21\xde\x5\x44\x5c\x4d\x12\x62\xa9\x10\x98\xdc\x68\xfb\x26\x29\x71\xbc\x23\x57\x4a\x7\xe7\xc3\x22\x44\x82\x91\xd8\xe4\x3\x88\x82\xad\xbe\x18\xc2\x4e\xef\x77";
static byte gMasterSessionSealPublicKey[CRYPTO_ENCRYPT_PUBLIC_SECRET_KEY_SIZE] = {0};
static byte gMasterSessionSealSecretKey[CRYPTO_ENCRYPT_PUBLIC_SECRET_KEY_SIZE] = {0};

static atomic bool gInitialized = false;

void cryptoInit(void) {
    assert(lifecycleInitialized() && !gInitialized);
    gInitialized = true;

    assert(!sodium_init());

    assert(!crypto_box_keypair(gMasterSessionSealPublicKey, gMasterSessionSealSecretKey));
}

bool cryptoInitialized(void) {
    return gInitialized;
}

const byte* cryptoMasterSignPublicKey(void) {
    assert(lifecycleInitialized() && gInitialized);
    return gMasterSignPublicKey;
}

const byte* cryptoMasterSessionSealPublicKey(void) {
    assert(lifecycleInitialized() && gInitialized);
    return gMasterSessionSealPublicKey;
}

void cryptoMasterSign(const byte* const message, const int size, byte* const signature) {
    assert(lifecycleInitialized() && gInitialized);

    xlong generatedSize;
    assert(!crypto_sign_detached(signature, &generatedSize, message, size, gMasterSignSecretKey));
    assert((int) generatedSize == CRYPTO_SIGNATURE_SIZE);
}

bool cryptoCheckMasterSigned(const byte* const message, const int size, const byte* const signature) {
    assert(lifecycleInitialized() && gInitialized);
    return !crypto_sign_verify_detached(signature, message, size, gMasterSignPublicKey);
}

void cryptoSeal(const byte* const message, const int size, byte* const sealedMessage, const byte* const sealPublicKey) {
    assert(lifecycleInitialized() && gInitialized);
    assert(!crypto_box_seal(sealedMessage, message, size, sealPublicKey));
}

bool cryptoMasterSessionUnseal(const byte* const sealedMessage, const int size, byte* const unsealedMessage) {
    assert(lifecycleInitialized() && gInitialized);
    return !crypto_box_seal_open(unsealedMessage, sealedMessage, size, gMasterSessionSealPublicKey, gMasterSessionSealSecretKey);
}

void cryptoMakeKeysForExchange(byte* const publicKey, byte* const secretKey) {
    assert(!crypto_kx_keypair(publicKey, secretKey));
}

bool cryptoExchangeKeys(
    const bool initiator,
    byte* const receiveKey,
    byte* const sendKey,
    const byte* const ownPublicKey,
    const byte* const ownSecretKey,
    const byte* const foreignPublicKey
) {
    return initiator
        ? !crypto_kx_client_session_keys(receiveKey, sendKey, ownPublicKey, ownSecretKey, foreignPublicKey)
        : !crypto_kx_server_session_keys(receiveKey, sendKey, ownPublicKey, ownSecretKey, foreignPublicKey);
}

void cryptoQuit(void) {
    assert(gInitialized);
    gInitialized = false;
}
