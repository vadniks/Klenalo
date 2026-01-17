
#include "../src/crypto/crypto.h"

[[gnu::section(".data")]] /*so it can be written initially*/ static const CryptoGenericKey PUBLIC_KEY, SECRET_KEY;
static const int DATA_SIZE = 13;
static const char DATA[DATA_SIZE] = "Hello World!";

static void sign(void) {
    CryptoSignedBundle* const bundle = xalloca(sizeof *bundle + DATA_SIZE);
    xmemcpy(bundle->data, DATA, DATA_SIZE);

    CryptoGenericKey publicKey;
    xmemcpy(&publicKey, &PUBLIC_KEY, sizeof publicKey);
    CryptoSignSecretKey secretKey;
    cryptoMakeSignKeypair(&publicKey, &secretKey);

    cryptoSign(bundle, DATA_SIZE, &secretKey);
    assert(cryptoSignVerify(bundle, DATA_SIZE, &publicKey));
}

static void seal(void) {
    CryptoPublicEncryptedBundle* const bundle = xalloca(sizeof *bundle + DATA_SIZE);
    xmemcpy(bundle->data, DATA, DATA_SIZE);

    cryptoPublicEncrypt(bundle, DATA_SIZE, &PUBLIC_KEY);
    assert(cryptoPublicDecrypt(bundle, DATA_SIZE, &PUBLIC_KEY, &SECRET_KEY));
}

static void singleCrypt(void) {
    CryptoSingleEncryptedBundle* const bundle = xalloca(sizeof *bundle + DATA_SIZE);
    xmemcpy(bundle->data, DATA, DATA_SIZE);
    cryptoRandomBytes(bundle->nonce, CRYPTO_SINGLE_CRYPT_NONCE_SIZE);

    cryptoSingleEncrypt(bundle, DATA_SIZE, &SECRET_KEY);
    assert(cryptoSingleDecrypt(bundle, DATA_SIZE, &SECRET_KEY));
}

static void streamCrypt(void) {
    CryptoStreamCoder encoder, decoder;
    CryptoStreamHeader header;
    CryptoGenericKey key;
    cryptoRandomBytes((byte*) &key, CRYPTO_GENERIC_KEY_SIZE);

    cryptoStreamCreateEncoder(&encoder, &header, &key);
    assert(cryptoStreamCreateDecoder(&decoder, &header, &key));

    const int chunksAmount = 5;
    CryptoStreamEncryptedChunkBundle* const chunks[chunksAmount] = {
        xalloca(sizeof *chunks[0] + 5),
        xalloca(sizeof *chunks[0] + 6),
        xalloca(sizeof *chunks[0] + 1),
        xalloca(sizeof *chunks[0] + 1),
        xalloca(sizeof *chunks[0] + 3),
    };
    xmemcpy(chunks[0]->data, "Abcd", 5);
    xmemcpy(chunks[1]->data, "01234", 6);
    xmemcpy(chunks[2]->data, " ", 1);
    xmemcpy(chunks[3]->data, "X", 1);
    xmemcpy(chunks[4]->data, "\xff\x05\x00", 3);

    cryptoStreamEncrypt(&encoder, chunks[0], 5);
    cryptoStreamEncrypt(&encoder, chunks[1], 6);
    cryptoStreamEncrypt(&encoder, chunks[2], 1);
    cryptoStreamEncrypt(&encoder, chunks[3], 1);
    cryptoStreamEncrypt(&encoder, chunks[4], 3);

    assert(cryptoStreamDecrypt(&decoder, chunks[0], 5));
    assert(!xmemcmp(chunks[0]->data, "Abcd", 5));
    assert(cryptoStreamDecrypt(&decoder, chunks[1], 6));
    assert(!xmemcmp(chunks[1]->data, "01234", 6));
    assert(cryptoStreamDecrypt(&decoder, chunks[2], 1));
    assert(!xmemcmp(chunks[2]->data, " ", 1));
    assert(cryptoStreamDecrypt(&decoder, chunks[3], 1));
    assert(!xmemcmp(chunks[3]->data, "X", 1));
    assert(cryptoStreamDecrypt(&decoder, chunks[4], 3));
    assert(!xmemcmp(chunks[4]->data, "\xff\x05\x00", 3));
}

static void nonce(void) {
    const int size = 16;
    byte nonce[size];
    xmemset(nonce + 1, 0xff, size - 1);
    nonce[0] = 0xfe;

    assert(!cryptoNonceIncrementOverflowChecked(nonce, size));
    assert(cryptoNonceIncrementOverflowChecked(nonce, size));
}

static void base64(void) {
    const byte data[DATA_SIZE] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 255, 254};
    byte data2[DATA_SIZE];

    const int resultSize = cryptoBase64ResultSize(DATA_SIZE);
    char result[resultSize];

    cryptoBase64Encode(data, DATA_SIZE, result, resultSize);
    assert((int) xstrnlen(result, 0xff) == resultSize - 1);

    assert(cryptoBase64Decode(result, resultSize, data2, DATA_SIZE) == DATA_SIZE);
    assert(!xmemcmp(data, data2, DATA_SIZE));
}

static void padding(void) {
    const int values[8] = {1, 15, 16, 17, 31, 32, 33, 46};
    for (int i = 0, size; i < (int) arraySize(values); i++) {
        size = values[i];

        byte data[size];
        xmemset(data, 'a', size);

        const int size2 = size + CRYPTO_PADDING_BLOCK_SIZE;
        byte padded[size2];

        const int actualPaddedSize = cryptoPaddingAdd(padded, size);
        assert(actualPaddedSize == (size / CRYPTO_PADDING_BLOCK_SIZE + 1) * CRYPTO_PADDING_BLOCK_SIZE);
        assert(cryptoPaddingRemovedSize(padded, actualPaddedSize) == size);
    }
}

static void hash(void) {
    byte buffer1[CRYPTO_HASH_LARGE_SIZE], buffer2[CRYPTO_HASH_LARGE_SIZE];
    cryptoHash(nullptr, (byte*) DATA, DATA_SIZE, buffer1, CRYPTO_HASH_LARGE_SIZE);

    CryptoHashState state;
    cryptoHash(&state, nullptr, 0, nullptr, CRYPTO_HASH_LARGE_SIZE);
    cryptoHash(&state, (byte*) "Hello ", 6, nullptr, 0);
    cryptoHash(&state, (byte*) "World!", 6, nullptr, 0);
    cryptoHash(&state, /*(byte[1]) {0}*/ &(byte) {}, 1, nullptr, 0);
    cryptoHash(&state, nullptr, 0, buffer2, CRYPTO_HASH_LARGE_SIZE);

    assert(!xmemcmp(buffer1, buffer2, CRYPTO_HASH_LARGE_SIZE));
}

void testCrypto(void) {
    cryptoInit();
    cryptoMakeKeypair((CryptoGenericKey*) &PUBLIC_KEY, (CryptoGenericKey*) &SECRET_KEY);

    sign();
    seal();
    singleCrypt();
    streamCrypt();
    nonce();
    base64();
    padding();
    hash();

    cryptoQuit();
}
