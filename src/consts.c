
#include "consts.h"

static ConstsLanguage gLanguage = CONSTS_LANGUAGE_EN;

void constsSetLanguage(const ConstsLanguage language) {
    gLanguage = language;
}

static const char* enString(const ConstsString string) {
    switch (string) {
        case SPLASH:
            return u8"Klenalo Copyright (C) 2024-2025 Vadim Nikolaev (https://github.com/vadniks)  \n"
                   u8"              This program comes with ABSOLUTELY NO WARRANTY;                \n"
                   u8"This is free software which is available under the terms of the GNU GPL 3.0";
        case TITLE:
            return u8"Klenalo";
        case WELCOME:
            return u8"Welcome!";
        case PASSWORD:
            return u8"Password";
        case SIGN_IN:
            return u8"Sign in";
        case REMEMBER_CREDENTIALS:
            return u8"Remember credentials";
        case IP_ADDRESS:
            return u8"IP Address";
        case NETWORK:
            return u8"Network";
    }
}

const char* constsString(const ConstsString string) {
    switch (gLanguage) {
        default:
            return enString(string);
    }
}
