
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

void cryptoQuit(void) {
    assert(gInitialized);
    gInitialized = false;
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

void cryptoRandomBytes(byte* const buffer, const int size) {
    assert(lifecycleInitialized() && gInitialized && size > 0);
    randombytes_buf(buffer, size);
}

void cryptoSingleEncrypt(CryptoSingleEncryptedBundle* const bundle, const int dataSize, const CryptoGenericKey* const key) {
    assert(lifecycleInitialized() && gInitialized && dataSize > 0);

    byte data[dataSize];
    xmemcpy(data, bundle->data, dataSize);

    assert(!crypto_secretbox_detached(
        bundle->data,
        bundle->mac,
        data,
        dataSize,
        bundle->nonce,
        (byte*) key
    ));
    sodium_memzero(data, dataSize);
}

bool cryptoSingleDecrypt(CryptoSingleEncryptedBundle* const bundle, const int dataSize, const CryptoGenericKey* const key) {
    assert(lifecycleInitialized() && gInitialized && dataSize > 0);

    byte data[dataSize];
    xmemcpy(data, bundle->data, dataSize);

    return !crypto_secretbox_open_detached(
        bundle->data,
        data,
        bundle->mac,
        dataSize,
        bundle->nonce,
        (byte*) key
    );
}

#ifdef DEBUG

static void tests(void) {
    CryptoGenericKey publicKey, secretKey;
    cryptoMakeKeypair(&publicKey, &secretKey);

    const int dataSize = 13;
    const char data[] = "Hello World!";

    {
        CryptoPublicEncryptedBundle* const bundle = xalloca(sizeof *bundle + dataSize);
        xmemcpy(bundle->data, data, dataSize);

        cryptoPublicEncrypt(bundle, dataSize, &publicKey);
        assert(cryptoPublicDecrypt(bundle, dataSize, &publicKey, &secretKey));
    }

    {
        CryptoSingleEncryptedBundle* const bundle = xalloca(sizeof *bundle + dataSize);
        xmemcpy(bundle->data, data, dataSize);
        cryptoRandomBytes(bundle->nonce, CRYPTO_SINGLE_CRYPT_NONCE_SIZE);

        cryptoSingleEncrypt(bundle, dataSize, &secretKey);
        assert(cryptoSingleDecrypt(bundle, dataSize, &secretKey));
    }
}

#endif
