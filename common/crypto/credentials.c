
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

void credentialsGlobalSignKeypair(CryptoGenericKey* const publicKey, CryptoSignSecretKey* const secretKey) {
    static CryptoGenericKey pk = {};
    static CryptoSignSecretKey sk = {};
    static bool loaded = false;

    if (loaded) {
        *publicKey = pk;
        *secretKey = sk;
        return;
    }

    constexpr char pkstr[] = "globalPublicSignKey", skstr[] = "globalSecretSignKey";

    const char* const pkpos = strstr(gCredentials.data, pkstr);
    assert(pkpos);
    hexdumpToBytes(&pk, pkpos + 1ul + strlen(pkstr), sizeof pk);
    *publicKey = pk;

    const char* const skpos = strstr(gCredentials.data, skstr);
    assert(skpos);
    hexdumpToBytes(&sk, skpos + 1ul + strlen(skstr), sizeof sk);
    *secretKey = sk;

    loaded = true;
}

void credentialsQuit() {
    assert(gInitialized);
    gInitialized = false;

    SDL_free(gCredentials.data);
}
