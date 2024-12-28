
#pragma once

enum : int {
    MAX_USERNAME_SIZE = 16,
    MAX_PASSWORD_SIZE = 16
};

typedef enum : char {
    CONSTS_LANGUAGE_EN
} ConstsLanguage;

typedef enum : int {
    TITLE,
    WELCOME,
    PASSWORD,
    SIGN_IN,
    REMEMBER_CREDENTIALS
} ConstsString;

void constsSetLanguage(const ConstsLanguage language);
const char* constsString(const ConstsString string);
