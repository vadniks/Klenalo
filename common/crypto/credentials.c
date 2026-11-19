
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

static byte charToHexDigit(const char c) {
    byte result = 0;
    if (c >= '0' && c <= '9') result = (byte) (c - '0');
    else if (c >= 'a' && c <= 'f') result = (byte) (c - 'a' + 10);
    else assert(false);
    return result;
}

static void hexdumpToBytes(byte* const buffer, const int bufferSize, const char* const string) {
    const int stringSize = bufferSize * 2;
    for (int i = 0; i < stringSize; i += 2) {
        const byte highHexDigit = charToHexDigit(string[i]);
        const byte lowHexDigit = charToHexDigit(string[i + 1]);
        buffer[i / 2] = (byte) (highHexDigit << (byte) 4) | lowHexDigit;
    }
}

static void findHexdumpedValue(void* const buffer, const int size, const char* const key) {
    const char* const position = strstr(gCredentials.data, key); // key=value, key=[chars], value=[bytes where each is represented by two hex digits ab=0xab]
    assert(position);
    hexdumpToBytes(buffer, size, position + 1ul + strlen(key));
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

    void* buffer;
    int size;
    const char* key;

    if (publicOrSecret) {
        buffer = &pk;
        size = sizeof pk;
        key = "globalPublicSignKey";
    } else {
        buffer = &sk;
        size = sizeof sk;
        key = "globalSecretSignKey";
    }

    findHexdumpedValue(buffer, size, key);

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
