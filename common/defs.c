
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include "defs.h"

void assert(const bool condition) {
    if (condition) return;
    abort(); // or __builtin_trap()
}

int xrand(const int min, const int max) {
    static bool initialized; // initialized to zero/false automatically

    if (!initialized) {
        initialized = true;
        srand((unsigned) time(nullptr));
    }

    return rand() % (max - min + 1) + min;
}

void printMemory(const void* const memory, const int size, const PrintMemoryMode mode) {
    const char* format, * divider;
    bool tryStr = false;

    switch (mode) {
        case PRINT_MEMORY_MODE_DEC:                  format = "%u";    divider = ", ";                break;
        case PRINT_MEMORY_MODE_HEX:                  format = "%x";    divider = ", ";                break;
        case PRINT_MEMORY_MODE_HEX_STR:              format = "\\x%x"; divider = "";                  break;
        case PRINT_MEMORY_MODE_STR:                  format = "%c";    divider = "";                  break;
        case PRINT_MEMORY_MODE_TRY_STR_HEX_FALLBACK:                                   tryStr = true; break;
    }

    if (tryStr) {
        for (int i = 0; i < size; i++) {
            const byte bt = ((const byte*) memory)[i];
            bt == ' ' || inRange('a', bt, 'z') || inRange('A', bt, 'Z') || inRange('0', bt, '9')
                ? printf("\033[1;32m%c\033[0m", bt) : printf("%x", bt);
            if (i < size - 1) printf(" ");
        }
    } else
        for (int i = 0; i < size; printf(format, ((const byte*) memory)[i++]), i < size ? printf("%s", divider) : 0) {}
    printf("\n");
}

int hashValue(const void* const value, const int size) {
    unsigned XXH32(const void* input, unsigned long length, unsigned seed);
    return (int) XXH32(value, size, 0);
}

unsigned long currentTimeMillis() {
    struct timespec timespec;
    assert(!clock_gettime(CLOCK_REALTIME, &timespec));
    return timespec.tv_sec * 1'000u + timespec.tv_nsec / 1'000'000u;
}
