
#pragma once

enum : int {
    MAX_USERNAME_SIZE = 16,
    MAX_PASSWORD_SIZE = 16
};

typedef enum : char {
    CONSTS_LANGUAGE_EN
} ConstsLanguage;

typedef enum : int {
    CONSTS_STRING_SPLASH,
    CONSTS_STRING_TITLE,
    CONSTS_STRING_WELCOME,
    CONSTS_STRING_PASSWORD,
    CONSTS_STRING_SIGN_IN,
    CONSTS_STRING_REMEMBER_CREDENTIALS,
    CONSTS_STRING_SUBNET_HOST_ADDRESS
} ConstsString;

void constsSetLanguage(const ConstsLanguage language);
const char* constsString(const ConstsString string);
