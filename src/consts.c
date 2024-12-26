
#include "consts.h"

static ConstsLanguage gLanguage = LANGUAGE_EN;

void constsSetLanguage(const ConstsLanguage language) {
    gLanguage = language;
}

static const char* enString(const ConstsString string) {
    switch (string) {
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
    }
}

const char* constsString(const ConstsString string) {
    switch (gLanguage) {
        default:
            return enString(string);
    }
}
