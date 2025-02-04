
#include "consts.h"

static ConstsLanguage gLanguage = CONSTS_LANGUAGE_EN;

void constsSetLanguage(const ConstsLanguage language) {
    gLanguage = language;
}

static const char* enString(const ConstsString string) {
    switch (string) {
        case CONSTS_STRING_SPLASH:
            return u8"Klenalo Copyright (C) 2024-2025 Vadim Nikolaev (https://github.com/vadniks)  \n"
                   u8"              This program comes with ABSOLUTELY NO WARRANTY;                \n"
                   u8"This is free software which is available under the terms of the GNU GPL 3.0";
        case CONSTS_STRING_TITLE:
            return u8"Klenalo";
        case CONSTS_STRING_WELCOME:
            return u8"Welcome!";
        case CONSTS_STRING_PASSWORD:
            return u8"Password";
        case CONSTS_STRING_SIGN_IN:
            return u8"Sign in";
        case CONSTS_STRING_REMEMBER_CREDENTIALS:
            return u8"Remember credentials";
        case CONSTS_STRING_IP_ADDRESS:
            return u8"IP Address";
        case CONSTS_STRING_NETWORK:
            return u8"Network";
    }
}

const char* constsString(const ConstsString string) {
    switch (gLanguage) {
        default:
            return enString(string);
    }
}
