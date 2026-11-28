// Copyright (C) 2026 Vadim Nikolaev (https://github.com/vadniks)
// SPDX-License-Identifier: AGPL-3.0-only

#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <string.h>
#include "credentials.h"

static atomic bool gInitialized = false;
static struct {char* data; unsigned size;} gCredentials = {nullptr, 0};

void credentialsInit() {
    assert(!gInitialized);
    gInitialized = true;

    const int fd = open("credentials.table", O_RDONLY);
    assert(fd > 0);

    struct stat st;
    assert(!fstat(fd, &st));

    assert(gCredentials.size = (unsigned) st.st_size);
    assert(gCredentials.data = mmap(nullptr, gCredentials.size, PROT_READ, MAP_PRIVATE, fd, 0));

    close(fd);
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

void credentialsGlobalSignKey(CryptoKey* nullable const key, const bool publicOrSecret) {
    static CryptoKey pk = {}, sk = {};
    static bool loadedPk = false, loadedSk = false;

    if (!key) {
        cryptoZeroMemory(&pk, sizeof pk);
        cryptoZeroMemory(&sk, sizeof sk);
        loadedPk = loadedSk = false;
        return;
    }

    if (publicOrSecret && loadedPk) {
        *key = pk;
        return;
    }
    if (!publicOrSecret && loadedSk) {
        *key = sk;
        return;
    }

    void* buffer;
    int size;
    const char* needle;

    if (publicOrSecret) {
        buffer = &pk;
        size = sizeof pk;
        needle = "globalPublicSignKey";
    } else {
        buffer = &sk;
        size = sizeof sk;
        needle = "globalSecretSignKey";
    }

    findHexdumpedValue(buffer, size, needle);

    if (publicOrSecret) {
        *key = pk;
        loadedPk = true;
    } else {
        *key = sk;
        loadedSk = true;
    }
}

void credentialsQuit() {
    assert(gInitialized);
    gInitialized = false;

    credentialsGlobalSignKey(nullptr, false);
    // cryptoZeroMemory(gCredentials.data, gCredentials.size); // unnecessary and impossible on readonly memory mappings
    assert(!munmap(gCredentials.data, gCredentials.size));
}
