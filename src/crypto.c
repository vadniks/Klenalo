
#include <sodium.h>
#include "lifecycle.h"
#include "crypto.h"

typedef unsigned long long xlong;

const int
    CRYPTO_PUBLIC_KEY_SIZE = crypto_sign_PUBLICKEYBYTES,
    CRYPTO_SECRET_KEY_SIZE = crypto_sign_SECRETKEYBYTES,
    CRYPTO_SIGNATURE_SIZE = crypto_sign_BYTES;

static atomic bool gInitialized = false;

static const byte gMasterPublicKey[CRYPTO_PUBLIC_KEY_SIZE] = "\xa9\x10\x98\xdc\x68\xfb\x26\x29\x71\xbc\x23\x57\x4a\x7\xe7\xc3\x22\x44\x82\x91\xd8\xe4\x3\x88\x82\xad\xbe\x18\xc2\x4e\xef\x77";
static const byte gMasterSecretKey[CRYPTO_SECRET_KEY_SIZE] = "\xde\x51\x0\xb3\xf3\x68\xf0\x93\x9c\x51\x0\x19\x86\x53\xc3\x99\xc9\xa7\xc2\x23\x9a\xa4\x46\x26\x21\xde\x5\x44\x5c\x4d\x12\x62\xa9\x10\x98\xdc\x68\xfb\x26\x29\x71\xbc\x23\x57\x4a\x7\xe7\xc3\x22\x44\x82\x91\xd8\xe4\x3\x88\x82\xad\xbe\x18\xc2\x4e\xef\x77";

void cryptoInit(void) {
    assert(lifecycleInitialized() && !gInitialized);
    gInitialized = true;

    assert(!sodium_init());
}

bool cryptoInitialized(void) {
    return gInitialized;
}

void cryptoMasterSign(const byte* const message, const int size, byte* const buffer) {
    xlong generatedSize;
    assert(!crypto_sign(buffer, &generatedSize, message, size, gMasterSecretKey));
    assert((int) generatedSize == CRYPTO_SIGNATURE_SIZE + size);
}

bool cryptoCheckMasterSigned(const byte* const signedMessage, const int size) {
    return !crypto_sign_open(nullptr, nullptr, signedMessage, size, gMasterPublicKey);
}

void cryptoQuit(void) {
    assert(gInitialized);
    gInitialized = false;
}
