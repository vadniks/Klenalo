
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

}

static void streamCrypt(void) {

}

static void nonce(void) {

}

static void base64(void) {

}

static void padding(void) {

}

static void hash(void) {

}

static bool lifecycleInitializedInterceptor(void) {
    return true;
}

void testCrypto(void) {
    cryptoInit();
    cryptoMakeKeypair((CryptoGenericKey*) &PUBLIC_KEY, (CryptoGenericKey*) &SECRET_KEY);

    sign();
    seal();

    cryptoQuit();
}
