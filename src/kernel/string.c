#include "string.h"

size_t strlen(const char* str) {
    size_t len = 0;
    while (str[len]) len++;
    return len;
}

char* strcpy(char* dst, const char* src) {
    char* orig = dst;
    while ((*dst++ = *src++));
    return orig;
}

char* strncpy(char* dst, const char* src, size_t num) {
    char* orig = dst;
    while (num > 0 && (*dst++ = *src++)) num--;
    while (num > 0) { *dst++ = '\0'; num--; }
    return orig;
}

char* strcat(char* dst, const char* src) {
    char* orig = dst;
    while (*dst) dst++;
    while ((*dst++ = *src++));
    return orig;
}

int strcmp(const char* str1, const char* str2) {
    while (*str1 && (*str1 == *str2)) { str1++; str2++; }
    return *(unsigned char*)str1 - *(unsigned char*)str2;
}

char* strchr(const char* str, int character) {
    while (*str) {
        if (*str == (char)character) return (char*)str;
        str++;
    }
    if (character == '\0') return (char*)str;
    return NULL;
}
void to_upper(char* str) {
    while (*str) {
        if (*str >= 'a' && *str <= 'z') *str -= 32;
        str++;
    }
}
