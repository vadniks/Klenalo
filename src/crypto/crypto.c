
#include <sodium.h>
#include "crypto.h"

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
    (CRYPTO_STREAM_MAC_SIZE == crypto_secretstream_xchacha20poly1305_ABYTES - 1) &
    (CRYPTO_SINGLE_CRYPT_MAC_SIZE == crypto_secretbox_MACBYTES) &
    (CRYPTO_SINGLE_CRYPT_NONCE_SIZE == crypto_secretbox_NONCEBYTES)
);

static atomic bool gInitialized = false;

void cryptoInit(void) {
    assert(!gInitialized);
    gInitialized = true;

    assert(!sodium_init()); // there's no quit counterpart function linke sodium_quit()
}

void cryptoQuit(void) {
    assert(gInitialized);
    gInitialized = false;
}

void cryptoMakeSignKeypair(CryptoGenericKey* const publicKey, CryptoSignSecretKey* const secretKey) {
    assert(gInitialized);
    assert(!crypto_sign_keypair((byte*) publicKey, (byte*) secretKey));
}

void cryptoSign(CryptoSignedBundle* const bundle, const int dataSize, const CryptoSignSecretKey* const secretKey) {
    assert(gInitialized && dataSize > 0);

    assert(!crypto_sign_detached(
        bundle->signature,
        nullptr,
        bundle->data,
        dataSize,
        (byte*) secretKey
    ));
}

bool cryptoSignVerify(CryptoSignedBundle* const bundle, const int dataSize, const CryptoGenericKey* const publicKey) {
    assert(gInitialized && dataSize > 0);

    return !crypto_sign_verify_detached(
        bundle->signature,
        bundle->data,
        dataSize,
        (byte*) publicKey
    );
}

void cryptoMakeKeypair(CryptoGenericKey* const publicKey, CryptoGenericKey* const secretKey) {
    assert(gInitialized);
    assert(!crypto_box_keypair((byte*) publicKey, (byte*) secretKey));
}

void cryptoPublicEncrypt(CryptoPublicEncryptedBundle* const bundle, const int dataSize, const CryptoGenericKey* const publicKey) {
    assert(gInitialized && dataSize > 0);

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
    assert(gInitialized && dataSize > 0);

    const int bundleSize = (int) sizeof *bundle + dataSize;

    CryptoPublicEncryptedBundle* const bundleCopy = xalloca2(bundleSize);
    xmemcpy((byte*) bundleCopy, (byte*) bundle, bundleSize);

    return !crypto_box_seal_open(bundle->data, (byte*) bundleCopy, bundleSize, (byte*) publicKey, (byte*) secretKey);
}

void cryptoSingleEncrypt(CryptoSingleEncryptedBundle* const bundle, const int dataSize, const CryptoGenericKey* const key) {
    assert(gInitialized && dataSize > 0);

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
    assert(gInitialized && dataSize > 0);

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

void cryptoStreamCreateEncoder(CryptoStreamCoder* const coder, CryptoStreamHeader* const header, const CryptoGenericKey* const key) {
    assert(gInitialized);
    assert(!crypto_secretstream_xchacha20poly1305_init_push((void*) coder, (void*) header, (void*) key));
}

bool cryptoStreamCreateDecoder(CryptoStreamCoder* const coder, const CryptoStreamHeader* const header, const CryptoGenericKey* const key) {
    assert(gInitialized);
    return !crypto_secretstream_xchacha20poly1305_init_pull((void*) coder, (void*) header, (void*) key);
}

void cryptoStreamEncrypt(CryptoStreamCoder* const coder, CryptoStreamEncryptedChunkBundle* const bundle, const int dataSize) {
    assert(gInitialized && dataSize > 0);

    const int encryptedSize = (int) sizeof *bundle + dataSize;
    byte encrypted[encryptedSize];

    assert(!crypto_secretstream_xchacha20poly1305_push(
        (void*) coder,
        encrypted,
        nullptr,
        bundle->data,
        dataSize,
        nullptr,
        0,
        crypto_secretstream_xchacha20poly1305_TAG_MESSAGE
    ));

    sodium_memzero(bundle->data, dataSize);

    bundle->tag = encrypted[0];
    xmemcpy(bundle->mac, encrypted + 1, CRYPTO_STREAM_MAC_SIZE);
    xmemcpy(bundle->data, encrypted + 1 + CRYPTO_STREAM_MAC_SIZE, dataSize);
}

bool cryptoStreamDecrypt(CryptoStreamCoder* const coder, CryptoStreamEncryptedChunkBundle* const bundle, const int dataSize) {
    assert(gInitialized && dataSize > 0);

    const int encryptedSize = (int) sizeof *bundle + dataSize;
    byte encrypted[encryptedSize];

    encrypted[0] = bundle->tag;
    xmemcpy(encrypted + 1, bundle->mac, CRYPTO_STREAM_MAC_SIZE);
    xmemcpy(encrypted + 1 + CRYPTO_STREAM_MAC_SIZE, bundle->data, dataSize);

    byte tag;
    const bool successful = !crypto_secretstream_xchacha20poly1305_pull(
        (void*) coder,
        bundle->data,
        nullptr,
        &tag,
        encrypted,
        encryptedSize,
        nullptr,
        0
    );

    assert(tag == crypto_secretstream_xchacha20poly1305_TAG_MESSAGE);
    return successful;
}

void cryptoRandomBytes(byte* const buffer, const int size) {
    assert(gInitialized && size > 0);
    randombytes_buf(buffer, size);
}

void cryptoZeroOutMemory(void* const memory, const int size) {
    assert(gInitialized && size > 0);
    sodium_memzero(memory, size);
}

bool cryptoNonceIncrementOverflowChecked(byte* const nonce, const int size) {
    assert(gInitialized && size > 0);
    sodium_increment(nonce, size);
    return sodium_is_zero(nonce, size);
}

int cryptoBase64ResultSize(const int binarySize) {
    assert(gInitialized && binarySize > 0);
    return (int) sodium_base64_encoded_len(binarySize, sodium_base64_VARIANT_URLSAFE);
}

void cryptoBase64Encode(const byte* const binary, const int binarySize, char* const string, const int stringSize) {
    assert(gInitialized && binarySize > 0 && stringSize > 0);
    assert(sodium_bin2base64(string, stringSize, binary, binarySize, sodium_base64_VARIANT_URLSAFE) == string);
}

int cryptoBase64Decode(const char* const string, const int stringSize, byte* const binary, const int binarySize) {
    assert(gInitialized && stringSize - 1 > 0 && binarySize > 0);

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

int cryptoPaddingAdd(byte* const padded, const int size) {
    assert(gInitialized && size > 0);

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
    assert(gInitialized && size > 0 && size % CRYPTO_PADDING_BLOCK_SIZE == 0);

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
    assert(gInitialized && (inRange((int) crypto_generichash_BYTES_MIN, hashSize, (int) crypto_generichash_BYTES_MAX) || !hashSize));

    if (!state && data && output)
        assert(!crypto_generichash(output, hashSize, data, dataSize, nullptr, 0));
    else if (state && !data && !output)
        assert(!crypto_generichash_init((void*) state, nullptr, 0, hashSize));
    else if (state && data && !output)
        assert(!crypto_generichash_update((void*) state, data, dataSize));
    else if (state && !data /*&& output*/)
        assert(!crypto_generichash_final((void*) state, output, hashSize));
    else
        assert(false);
}
