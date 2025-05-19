// TODO: allocations tracker via return addresses and (arena based) hashtable
#include <sodium.h>
#include "lifecycle.h"
#include "crypto.h"

typedef unsigned long long xulong;
// TODO: dbus, tray
staticAssert(
    (CRYPTO_SIGN_SECRET_KEY_SIZE == crypto_sign_SECRETKEYBYTES) &
    (CRYPTO_SIGNATURE_SIZE == crypto_sign_BYTES) &
        (crypto_sign_BYTES == crypto_sign_SECRETKEYBYTES) &
    (CRYPTO_GENERIC_KEY_SIZE == crypto_box_PUBLICKEYBYTES) &
        (crypto_box_PUBLICKEYBYTES == crypto_box_SECRETKEYBYTES) &
        (crypto_box_PUBLICKEYBYTES == crypto_sign_PUBLICKEYBYTES) &
        (crypto_box_PUBLICKEYBYTES == crypto_sign_SECRETKEYBYTES / 2) &
        (crypto_box_PUBLICKEYBYTES == crypto_secretbox_KEYBYTES) &
        (crypto_box_PUBLICKEYBYTES == crypto_kx_PUBLICKEYBYTES) &
        (crypto_box_PUBLICKEYBYTES == crypto_kx_SECRETKEYBYTES) &
        (crypto_box_PUBLICKEYBYTES == crypto_kx_SESSIONKEYBYTES) &
        (crypto_box_PUBLICKEYBYTES == crypto_secretstream_xchacha20poly1305_KEYBYTES) &
        (CRYPTO_SEAL_SIZE == crypto_box_SEALBYTES) &
        (CRYPTO_STREAM_CODER_SIZE == sizeof(crypto_secretstream_xchacha20poly1305_state)) &
        (CRYPTO_STREAM_HEADER_SIZE == crypto_secretstream_xchacha20poly1305_HEADERBYTES) &
        (CRYPTO_STREAM_AUTH_TAG_SIZE == crypto_secretstream_xchacha20poly1305_ABYTES) &
    (CRYPTO_SINGLE_CRYPT_AUTH_TAG_SIZE == crypto_secretbox_MACBYTES) &
    (CRYPTO_SINGLE_CRYPT_NONCE_SIZE == crypto_secretbox_NONCEBYTES)
);

static atomic bool gInitialized = false;

void cryptoInit(void) {
    assert(lifecycleInitialized() && !gInitialized);
    gInitialized = true;

    assert(!sodium_init());

    CryptoGenericKey p, s;
    crypto_box_keypair(&p, &s);

    CryptoSealed* const sealed = xalloca(sizeof *sealed + 10);
    xmemcpy(sealed->message, "abcdefghij", 10);
    cryptoSeal(&p, sealed, 10);
    assert(cryptoUnseal(&p, &s, sealed, 10));
    assert(!xmemcmp(sealed->message, "abcdefghij", 10));

    CryptoGenericKey key = {0};

    CryptoSingleEncrypted* const encrypted = xalloca(sizeof *encrypted + 10);
    xmemcpy(encrypted->message, "0123456789", 10);
    cryptoSingleEncrypt(&key, encrypted, 10, false);
    assert(cryptoSingleDecrypt(&key, encrypted, 10));
    debugArgs("%s", encrypted->message)
}

bool cryptoInitialized(void) {
    return gInitialized;
}

void cryptoSign(const CryptoSignSecretKey* const secretKey, CryptoSigned* const xSigned, const int messageSize) {
    assert(lifecycleInitialized() && gInitialized);

    xulong generatedSize;
    assert(!crypto_sign_detached(xSigned->signature, &generatedSize, xSigned->message, messageSize, (void*) secretKey));
    assert((int) generatedSize == CRYPTO_SIGNATURE_SIZE);
}

bool cryptoCheckSigned(const CryptoGenericKey* const publicKey, const CryptoSigned* const xSigned, const int messageSize) {
    assert(lifecycleInitialized() && gInitialized);
    return !crypto_sign_verify_detached(xSigned->signature, xSigned->message, messageSize, (void*) publicKey);
}

void cryptoSeal(const CryptoGenericKey* const publicKey, CryptoSealed* const sealed, const int messageSize) {
    assert(lifecycleInitialized() && gInitialized);
    assert(!crypto_box_seal((void*) sealed, sealed->message, messageSize, (void*) publicKey));
}

bool cryptoUnseal(const CryptoGenericKey* const publicKey, const CryptoGenericKey* const secretKey, CryptoSealed* const sealed, const int messageSize) {
    assert(lifecycleInitialized() && gInitialized);
    return !crypto_box_seal_open(sealed->message, (void*) sealed, CRYPTO_SEAL_SIZE + messageSize, (void*) publicKey, (void*) secretKey);
}

void cryptoMakeStreamCoderForEncryption(const CryptoGenericKey* const key, CryptoStreamCoder* const coder, CryptoStreamHeader* const header) {
    assert(lifecycleInitialized() && gInitialized);
    assert(!crypto_secretstream_xchacha20poly1305_init_push((crypto_secretstream_xchacha20poly1305_state*) coder, (void*) header, (void*) key));
}

bool cryptoMakeStreamCoderForDecryption(const CryptoGenericKey* const key, const CryptoStreamHeader* const header, CryptoStreamCoder* const coder) {
    assert(lifecycleInitialized() && gInitialized);
    return !crypto_secretstream_xchacha20poly1305_init_pull((crypto_secretstream_xchacha20poly1305_state*) coder, (void*) header, (void*) key);
}

void cryptoStreamEncrypt(CryptoStreamCoder* const coder, CryptoStreamEncryptedChunk* const chunk, const int messageSize) {
    assert(lifecycleInitialized() && gInitialized);
    xulong generatedSize;

    assert(!crypto_secretstream_xchacha20poly1305_push(
        (crypto_secretstream_xchacha20poly1305_state*) coder,
        (void*) chunk,
        &generatedSize,
        (void*) chunk,
        messageSize,
        nullptr,
        0,
        crypto_secretstream_xchacha20poly1305_TAG_MESSAGE
    ));
    assert((int) generatedSize == messageSize + CRYPTO_STREAM_AUTH_TAG_SIZE);
}

bool cryptoStreamDecrypt(CryptoStreamCoder* const coder, CryptoStreamEncryptedChunk* const chunk, const int messageSize) {
    assert(lifecycleInitialized() && gInitialized);

    xulong generatedSize;
    byte tag;

    const bool result = crypto_secretstream_xchacha20poly1305_pull(
        (crypto_secretstream_xchacha20poly1305_state*) coder,
        (void*) chunk,
        &generatedSize,
        &tag,
        (void*) chunk,
        CRYPTO_STREAM_AUTH_TAG_SIZE + messageSize,
        nullptr,
        0
    );
    assert(result && (int) generatedSize == messageSize - CRYPTO_STREAM_AUTH_TAG_SIZE || !result);
    assert(tag == crypto_secretstream_xchacha20poly1305_TAG_MESSAGE);

    return result;
}

void cryptoZeroOutMemory(void* const memory, const int size) {
    assert(lifecycleInitialized() && gInitialized);
    sodium_memzero(memory, size);
}

int cryptoAddPadding(byte* const message, const int size) {
    assert(lifecycleInitialized() && gInitialized);
    unsigned long generatedSize;

    assert(!sodium_pad(
        &generatedSize,
        message,
        size,
        CRYPTO_PADDING_BLOCK_SIZE,
        size + CRYPTO_PADDING_BLOCK_SIZE
    ));

    assert((int) generatedSize >= size);
    return (int) generatedSize;
}

int cryptoRemovePadding(byte* const message, const int size) {
    assert(lifecycleInitialized() && gInitialized);

    assert(size > 0 && size % CRYPTO_PADDING_BLOCK_SIZE == 0);
    unsigned long generatedSize;

    const bool successful = !sodium_unpad(&generatedSize, message, size, CRYPTO_PADDING_BLOCK_SIZE);

    if (successful) {
        assert((int) generatedSize <= size);
        return (int) generatedSize;
    } else
        return 0;
}

void cryptoSingleEncrypt(CryptoGenericKey* const key, CryptoSingleEncrypted* const encrypted, const int messageSize, const bool makeNonce) {
    assert(lifecycleInitialized() && gInitialized);

    byte message[messageSize];
    xmemcpy(message, encrypted->message, messageSize);

    if (makeNonce) randombytes_buf(encrypted->nonce, CRYPTO_SINGLE_CRYPT_NONCE_SIZE);
    assert(!crypto_secretbox_easy(encrypted->authTag, message, messageSize, encrypted->nonce, (void*) key));
}

bool cryptoSingleDecrypt(CryptoGenericKey* const key, CryptoSingleEncrypted* const encrypted, const int messageSize) {
    assert(lifecycleInitialized() && gInitialized);

    byte xEncrypted[sizeof *encrypted + messageSize];
    xmemcpy(xEncrypted, encrypted, sizeof(xEncrypted));

    return !crypto_secretbox_open(encrypted->message, xEncrypted + CRYPTO_SINGLE_CRYPT_NONCE_SIZE, CRYPTO_SINGLE_CRYPT_AUTH_TAG_SIZE + messageSize, encrypted->nonce, (void*) key);
}

void cryptoQuit(void) {
    assert(gInitialized);
    gInitialized = false;
}
