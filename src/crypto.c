
#include <sodium.h>
#include "lifecycle.h"
#include "crypto.h"

// TODO: staticAssert(<sizes> = <library's defined>);

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

void cryptoMakeSignKeypair(CryptoGenericKey* const publicKey, CryptoSignSecretKey* const secretKey) {
    assert(lifecycleInitialized() && gInitialized);
    assert(!crypto_sign_keypair((byte*) publicKey, (byte*) secretKey));
}

void cryptoSign(CryptoSignedBundle* const bundle, const int dataSize, const CryptoSignSecretKey* const secretKey) {
    assert(lifecycleInitialized() && gInitialized && dataSize > 0);

    assert(!crypto_sign_detached(
        bundle->signature,
        nullptr,
        bundle->data,
        dataSize,
        (byte*) secretKey
    ));
}

bool cryptoSignVerify(CryptoSignedBundle* const bundle, const int dataSize, const CryptoGenericKey* const publicKey) {
    assert(lifecycleInitialized() && gInitialized && dataSize > 0);

    return !crypto_sign_verify_detached(
        bundle->signature,
        bundle->data,
        dataSize,
        (byte*) publicKey
    );
}

void cryptoMakeKeypair(CryptoGenericKey* const publicKey, CryptoGenericKey* const secretKey) {
    assert(lifecycleInitialized() && gInitialized);
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

//

void cryptoRandomBytes(byte* const buffer, const int size) {
    assert(lifecycleInitialized() && gInitialized && size > 0);
    randombytes_buf(buffer, size);
}

void cryptoZeroOutMemory(void* const memory, const int size) {
    assert(lifecycleInitialized() && gInitialized && size > 0);
    sodium_memzero(memory, size);
}

bool cryptoNonceIncrementOverflowChecked(byte* const nonce, const int size) {
    assert(lifecycleInitialized() && gInitialized && size > 0);
    sodium_increment(nonce, size);
    return sodium_is_zero(nonce, size);
}

int cryptoBase64ResultSize(const int binarySize) {
    assert(lifecycleInitialized() && gInitialized && binarySize > 0);
    return (int) sodium_base64_encoded_len(binarySize, sodium_base64_VARIANT_URLSAFE);
}

void cryptoBase64Encode(const byte* const binary, const int binarySize, char* const string, const int stringSize) {
    assert(lifecycleInitialized() && gInitialized && binarySize > 0 && stringSize > 0);
    assert(sodium_bin2base64(string, stringSize, binary, binarySize, sodium_base64_VARIANT_URLSAFE) == string);
}

int cryptoBase64Decode(const char* const string, const int stringSize, byte* const binary, const int binarySize) {
    assert(lifecycleInitialized() && gInitialized && stringSize - 1 > 0 && binarySize > 0);

    unsigned long resultSize = 0;
    return !sodium_base642bin(
        binary,
        binarySize,
        string,
        stringSize - 1,
        nullptr,
        &resultSize,
        nullptr,
        sodium_base64_VARIANT_URLSAFE
    ) ? (int) resultSize : -1;
}

int cryptoPaddingAdd(const byte* const original, const int size, byte* const padded) {
    assert(lifecycleInitialized() && gInitialized && size > 0);

    unsigned long newSize = 0;
    assert(!sodium_pad(
        &newSize,
        padded,
        size,
        CRYPTO_PADDING_BLOCK_SIZE,
        size + CRYPTO_PADDING_BLOCK_SIZE
    ));
    assert((int) newSize >= size && (int) newSize <= size + CRYPTO_PADDING_BLOCK_SIZE && (int) newSize % CRYPTO_PADDING_BLOCK_SIZE == 0);

    return (int) newSize;
}

int cryptoPaddingRemovedSize(const byte* const padded, const int size) {
    assert(lifecycleInitialized() && gInitialized && size > 0 && size % CRYPTO_PADDING_BLOCK_SIZE == 0);

    unsigned long originalSize = 0;
    return
        !sodium_unpad(&originalSize, padded, size, CRYPTO_PADDING_BLOCK_SIZE)
        && (int) originalSize <= size && originalSize > 0 ? (int) originalSize : -1;
}

void cryptoHash(
    CryptoHashState* nullable const state,
    const byte* nullable const data,
    const int dataSize,
    byte* nullable const output,
    const int hashSize
) {
    assert(lifecycleInitialized() && gInitialized && (inRange((int) crypto_generichash_BYTES_MIN, hashSize, (int) crypto_generichash_BYTES_MAX) || !hashSize));

    if (!state && data && output)
        assert(!crypto_generichash(output, hashSize, data, dataSize, nullptr, 0));
    else if (state && !data && !output)
        assert(!crypto_generichash_init((void*) state, nullptr, 0, hashSize));
    else if (state && data && !output)
        assert(!crypto_generichash_update((void*) state, data, dataSize));
    else if (state && !data && output)
        assert(!crypto_generichash_final((void*) state, output, hashSize));
    else
        assert(false);
}

#ifdef DEBUG

static void tests(void) {
    CryptoGenericKey publicKey, secretKey;
    cryptoMakeKeypair(&publicKey, &secretKey);

    const int dataSize = 13;
    const char data[] = "Hello World!";

    {
        CryptoSignedBundle* const bundle = xalloca(sizeof *bundle + dataSize);
        xmemcpy(bundle->data, data, dataSize);

        CryptoSignSecretKey secretKey;
        cryptoMakeSignKeypair(&publicKey, &secretKey);

        cryptoSign(bundle, dataSize, &secretKey);
        assert(cryptoSignVerify(bundle, dataSize, &publicKey));
    }
    cryptoMakeKeypair(&publicKey, &secretKey);

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

    {
        const int size = 16;
        byte nonce[size];
        xmemset(nonce + 1, 0xff, size - 1);
        nonce[0] = 0xfe;

        assert(!cryptoNonceIncrementOverflowChecked(nonce, size));
        assert(cryptoNonceIncrementOverflowChecked(nonce, size));
    }

    {
        byte data[dataSize] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 255, 254}, data2[dataSize];

        const int resultSize = cryptoBase64ResultSize(dataSize);
        char result[resultSize];

        cryptoBase64Encode(data, dataSize, result, resultSize);
        assert((int) xstrnlen(result, 0xff) == resultSize - 1);

        assert(cryptoBase64Decode(result, resultSize, data2, dataSize) == dataSize);
        assert(!xmemcmp(data, data2, dataSize));
    }

    {
        const int values[8] = {1, 15, 16, 17, 31, 32, 33, 46};
        for (int i = 0, size; i < (int) arraySize(values); i++) {
            size = values[i];

            byte data[size];
            xmemset(data, 'a', size);

            const int size2 = size + CRYPTO_PADDING_BLOCK_SIZE;
            byte padded[size2];

            const int actualPaddedSize = cryptoPaddingAdd(data, size, padded);
            assert(actualPaddedSize == (size / CRYPTO_PADDING_BLOCK_SIZE + 1) * CRYPTO_PADDING_BLOCK_SIZE);
            assert(cryptoPaddingRemovedSize(padded, actualPaddedSize) == size);
        }
    }

    {
        byte buffer1[CRYPTO_HASH_LARGE_SIZE], buffer2[CRYPTO_HASH_LARGE_SIZE];
        cryptoHash(nullptr, (byte*) data, dataSize, buffer1, CRYPTO_HASH_LARGE_SIZE);

        CryptoHashState state;
        cryptoHash(&state, nullptr, 0, nullptr, CRYPTO_HASH_LARGE_SIZE);
        cryptoHash(&state, (byte*) "Hello ", 6, nullptr, 0);
        cryptoHash(&state, (byte*) "World!", 6, nullptr, 0);
        cryptoHash(&state, (byte[1]) {0}, 1, nullptr, 0);
        cryptoHash(&state, nullptr, 0, buffer2, CRYPTO_HASH_LARGE_SIZE);

        assert(!xmemcmp(buffer1, buffer2, CRYPTO_HASH_LARGE_SIZE));
    }
}

#endif
