
#include <string.h>
#include "defs.h"
#include "consts.h"

static ConstsLanguage gLanguage = CONSTS_LANGUAGE_EN;
static char gConcatenationBuffer[64] = {0};

void constsSetLanguage(const ConstsLanguage language) {
    gLanguage = language;
}

static const char* enString(const ConstsString string) {
    switch (string) {
        case CONSTS_STRING_SPLASH:
            return "Klenalo Copyright (C) 2024-2025 Vadim Nikolaev (https://github.com/vadniks)  \n"
                   "              This program comes with ABSOLUTELY NO WARRANTY;                \n"
                   "This is free software which is available under the terms of the GNU GPL 3.0";
        case CONSTS_STRING_TITLE:
            return "Klenalo"; // only ascii chars here!
        case CONSTS_STRING_WELCOME:
            return "Welcome!";
        case CONSTS_STRING_PASSWORD:
            return "Password";
        case CONSTS_STRING_SIGN_IN:
            return "Sign in";
        case CONSTS_STRING_REMEMBER_CREDENTIALS:
            return "Remember credentials";
        case CONSTS_STRING_SUBNET_HOST_ADDRESS:
            return "Subnet host address";
    }
}

const char* constsString(const ConstsString string) {
    switch (gLanguage) {
        default:
            return enString(string);
    }
}

const char* constsConcatenateTitleWith(const char* const added) {
    xmemset(gConcatenationBuffer, 0, sizeof gConcatenationBuffer);
    strncpy(gConcatenationBuffer, constsString(CONSTS_STRING_TITLE), sizeof gConcatenationBuffer);
    strncat(gConcatenationBuffer, added, sizeof(gConcatenationBuffer) - strlen(gConcatenationBuffer) - 1);
    return gConcatenationBuffer;
}
