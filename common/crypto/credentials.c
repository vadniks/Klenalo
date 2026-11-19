// Copyright (C) 2026 Vadim Nikolaev (https://github.com/vadniks)
// SPDX-License-Identifier: AGPL-3.0-only

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

static byte asciiCharToHexDigit(const char c) {
    byte result = 0;
    if (inRange('0', c, '9')) result = (byte) (c - '0');
    else if (inRange('a', c, 'f')) result = (byte) (c - 'a' + 10);
    else assert(false);
    return result;
}

static void hexdumpToBytes(byte* const buffer, const int bufferSize, const char* const string) {
    const int stringSize = bufferSize * 2;
    for (int i = 0; i < stringSize; i += 2) {
        const byte highHexDigit = asciiCharToHexDigit(string[i]);
        const byte lowHexDigit = asciiCharToHexDigit(string[i + 1]);
        buffer[i / 2] = (byte) (highHexDigit << (byte) 4) | lowHexDigit;
    }
}

static void findHexdumpedValue(void* const buffer, const int size, const char* const key) {
    const char* const position = strstr(gCredentials.data, key); // key=value, key=[chars], value=[bytes where each is represented by two hex digits ab=0xab]
    assert(position);
    hexdumpToBytes(buffer, size, position + 1ul + strlen(key));
}

void credentialsGlobalSignKey(CredentialsSignKey* nullable const signKey, const bool publicOrSecret) {
    static CryptoGenericKey pk = {};
    static CryptoSignSecretKey sk = {};
    static bool loadedPk = false, loadedSk = false;

    if (!signKey) {
        cryptoZeroMemory(&pk, sizeof pk);
        cryptoZeroMemory(&sk, sizeof sk);
        loadedPk = loadedSk = false;
        return;
    }

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

    credentialsGlobalSignKey(nullptr, false);
    cryptoZeroMemory(gCredentials.data, gCredentials.size);
    SDL_free(gCredentials.data);
}
