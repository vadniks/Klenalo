
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

void printMemory(const void* const memory, const int size, const PrintMemoryMode mode, const bool newline) {
    const char* format, * divider;
    bool tryStr = false;
    int wordSize = 1;

    switch (mode) {
        case PRINT_MEMORY_MODE_DEC:                  format = "%u";    divider = ", ";                             break;
        case PRINT_MEMORY_MODE_HEX:                  format = "%.2x";  divider = ", ";                             break;
        case PRINT_MEMORY_MODE_HEXDUMP:              format = "%.2x";  divider = "";                               break;
        case PRINT_MEMORY_MODE_BINARY:               format = "%.8b";  divider = "_";                              break;
        case PRINT_MEMORY_MODE_HEX_STR:              format = "\\x%x"; divider = "";                               break;
        case PRINT_MEMORY_MODE_STR:                  format = "%c";    divider = "";                               break;
        case PRINT_MEMORY_MODE_IPV6:                 format = "%.2x";  divider = ":"; wordSize = 2;                break;
        case PRINT_MEMORY_MODE_TRY_STR_HEX_FALLBACK:                                                tryStr = true; break;
        default: assert(false);
    }

    if (tryStr) {
        for (int i = 0; i < size; i++) {
            const byte bt = ((const byte*) memory)[i];
            bt == ' ' || inRange('a', bt, 'z') || inRange('A', bt, 'Z') || inRange('0', bt, '9')
                ? printf("\033[1;32m%c\033[0m", bt) : printf("%x", bt);
            if (i < size - 1) printf(" ");
        }
    } else
        for (int i = 0; i < size; printf(format, ((const byte*) memory)[i++]), i < size && !(i % wordSize) ? printf("%s", divider) : 0) {}
    if (newline) printf("\n");
}

int hashValue(const void* const value, const int size) {
    unsigned XXH32(const void* input, ulong length, unsigned seed);
    return (int) XXH32(value, size, 0);
}

ulong currentTimeMillis() {
    struct timespec timespec;
    assert(!clock_gettime(CLOCK_REALTIME, &timespec));
    return (ulong) timespec.tv_sec * 1'000ul + (ulong) timespec.tv_nsec / 1'000'000ul;
}

bool compareXmmVectors(const vec16aq vec1, const vec16aq vec2) {
    const auto result = (vec16aq) (vec1 == vec2);
    return result[0] == -1l && result[1] == -1l;
}
