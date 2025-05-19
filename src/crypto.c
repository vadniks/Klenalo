
#include <sodium.h>
#include "lifecycle.h"
#include "crypto.h"

static atomic bool gInitialized = false;

static void tests(void);

void cryptoInit(void) {
    assert(lifecycleInitialized() && !gInitialized);
    gInitialized = true;

    assert(!sodium_init());

#ifdef DEBUG
    tests();
#endif
}

void cryptoMakeKeypair(CryptoGenericKey* const publicKey, CryptoGenericKey* const secretKey) {
    assert(!crypto_box_keypair((byte*) publicKey, (byte*) secretKey));
}

void cryptoPublicEncrypt(CryptoPublicEncryptedBundle* const bundle, const int dataSize, const CryptoGenericKey* const publicKey) {
    assert(lifecycleInitialized() && gInitialized && dataSize > 0);

    byte data[dataSize];
    xmemcpy(data, bundle->data, dataSize);

    assert(!crypto_box_seal((byte*) bundle, data, dataSize, (byte*) publicKey));
    sodium_memzero(data, dataSize);
}

bool cryptoPublicDecrypt(
    CryptoPublicEncryptedBundle* const bundle,
    const int dataSize,
    const CryptoGenericKey* const publicKey,
    const CryptoGenericKey* const secretKey
) {
    assert(lifecycleInitialized() && gInitialized && dataSize > 0);

    const int bundleSize = (int) sizeof *bundle + dataSize;

    byte bundleCopy[bundleSize];
    xmemcpy(bundleCopy, bundle, bundleSize);

    return !crypto_box_seal_open(bundle->data, bundleCopy, bundleSize, (byte*) publicKey, (byte*) secretKey);
}

void cryptoQuit(void) {
    assert(gInitialized);
    gInitialized = false;
}

#ifdef DEBUG

static void tests(void) {
    CryptoGenericKey pk, sk;
    cryptoMakeKeypair(&pk, &sk);

    const int dataSize = 13;
    CryptoPublicEncryptedBundle* const bundle = xalloca(sizeof(CryptoPublicEncryptedBundle) + dataSize);
    xmemcpy(bundle->data, "Hello World!", dataSize);

    cryptoPublicEncrypt(bundle, dataSize, &pk);
    assert(cryptoPublicDecrypt(bundle, dataSize, &pk, &sk));
}

#endif
