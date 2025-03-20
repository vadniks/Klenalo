
#include <sodium.h>
#include "lifecycle.h"
#include "crypto.h"

typedef unsigned long long xlong;

const int
    CRYPTO_SIGN_PUBLIC_KEY_SIZE = crypto_sign_PUBLICKEYBYTES,
    CRYPTO_SIGN_SECRET_KEY_SIZE = crypto_sign_SECRETKEYBYTES,
    CRYPTO_SIGNATURE_SIZE = crypto_sign_BYTES;

static atomic bool gInitialized = false;

// TODO: will be replaced with config file where users can set the keys themselves
static const byte gMasterSignPublicKey[CRYPTO_SIGN_PUBLIC_KEY_SIZE] = "\xa9\x10\x98\xdc\x68\xfb\x26\x29\x71\xbc\x23\x57\x4a\x7\xe7\xc3\x22\x44\x82\x91\xd8\xe4\x3\x88\x82\xad\xbe\x18\xc2\x4e\xef\x77";
static const byte gMasterSignSecretKey[CRYPTO_SIGN_SECRET_KEY_SIZE] = "\xde\x51\x0\xb3\xf3\x68\xf0\x93\x9c\x51\x0\x19\x86\x53\xc3\x99\xc9\xa7\xc2\x23\x9a\xa4\x46\x26\x21\xde\x5\x44\x5c\x4d\x12\x62\xa9\x10\x98\xdc\x68\xfb\x26\x29\x71\xbc\x23\x57\x4a\x7\xe7\xc3\x22\x44\x82\x91\xd8\xe4\x3\x88\x82\xad\xbe\x18\xc2\x4e\xef\x77";

void cryptoInit(void) {
    assert(lifecycleInitialized() && !gInitialized);
    gInitialized = true;

    assert(!sodium_init());
}

bool cryptoInitialized(void) {
    return gInitialized;
}

void cryptoMasterSign(const byte* const message, const int size, byte* const signedMessage) {
    assert(lifecycleInitialized() && gInitialized);

    xlong generatedSize;
    assert(!crypto_sign(signedMessage, &generatedSize, message, size, gMasterSignSecretKey));
    assert((int) generatedSize == CRYPTO_SIGNATURE_SIZE + size);
}

bool cryptoCheckMasterSigned(const byte* const signedMessage, const int size) {
    assert(lifecycleInitialized() && gInitialized);
    return !crypto_sign_open(nullptr, nullptr, signedMessage, size, gMasterSignPublicKey);
}

void cryptoMasterSeal(const byte* const message, const int size, byte* const sealedMessage) {

}

bool cryptoMasterUnseal(const byte* const sealedMessage, const int size, byte* const unsealedMessage) {

}

void cryptoQuit(void) {
    assert(gInitialized);
    gInitialized = false;
}
