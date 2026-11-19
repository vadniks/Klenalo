
#include <SDL3/SDL_iostream.h>
#include <string.h>
#include "credentials.h"

static atomic bool gInitialized = false;
static struct {char* data; unsigned size;} gCredentials = {nullptr, 0};

void credentialsInit() {
    assert(!gInitialized);
    gInitialized = true;

    ulong size;
    assert(gCredentials.data = SDL_LoadFile("credentials.table", &size));
    assert(gCredentials.size = (unsigned) size);
}

static void hexdumpToBytes(void* const buffer, const char* const string, const int size) {
    constexpr char alphabet[] = "0123456789abcdef";
    const byte numbers[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};

    assert(size % 2 == 0);

    for (int srcChar = 0, dstByte = 0; dstByte < size; dstByte++) {
        byte next = numbers[(byte) (strchr(alphabet, string[srcChar]) - alphabet)] * 0x10;
        next += (byte) numbers[(strchr(alphabet, string[srcChar + 1]) - alphabet)];
        srcChar += 2;
        ((byte*) buffer)[dstByte] = next;
    }
}

static void findHexdumpedValue(void* const buffer, const int size, const char* const needle) {
    const char* const position = strstr(gCredentials.data, needle);
    assert(position);
    hexdumpToBytes(buffer, position + 1ul + strlen(needle), size);
}

void credentialsGlobalSignKey(CredentialsSignKey* const signKey, const bool publicOrSecret) {
    static CryptoGenericKey pk = {};
    static CryptoSignSecretKey sk = {};
    static bool loadedPk = false, loadedSk = false;

    if (publicOrSecret && loadedPk) {
        signKey->publicKey = pk;
        return;
    }
    if (!publicOrSecret && loadedSk) {
        signKey->secretKey = sk;
        return;
    }

    findHexdumpedValue(
        publicOrSecret ? (void*) &pk : (void*) &sk,
        publicOrSecret ? sizeof pk : sizeof sk,
        publicOrSecret ? "globalPublicSignKey" : "globalSecretSignKey"
    );
    if (publicOrSecret) {
        signKey->publicKey = pk;
        loadedPk = true;
    } else {
        signKey->secretKey = sk;
        loadedSk = true;
    }
}

void credentialsQuit() {
    assert(gInitialized);
    gInitialized = false;

    SDL_free(gCredentials.data);
}
