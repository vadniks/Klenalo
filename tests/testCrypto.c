
#include "../src/crypto/crypto.h"

[[gnu::section(".data")]] static const CryptoGenericKey gPublicKey, gSecretKey;
static const int gDataSize = 13;
static const char gData[] = "Hello World!";

static void sign(void) {
    CryptoSignedBundle* const bundle = xalloca(sizeof *bundle + gDataSize);
    xmemcpy(bundle->data, gData, gDataSize);

    CryptoGenericKey publicKey;
    xmemcpy(&publicKey, &gPublicKey, sizeof publicKey);
    CryptoSignSecretKey secretKey;
    cryptoMakeSignKeypair(&publicKey, &secretKey);

    cryptoSign(bundle, gDataSize, &secretKey);
    assert(cryptoSignVerify(bundle, gDataSize, &publicKey));
}

static void crypt(void) {

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

    cryptoMakeKeypair((CryptoGenericKey*) &gPublicKey, (CryptoGenericKey*) &gSecretKey);

    sign();

    cryptoQuit();
}
