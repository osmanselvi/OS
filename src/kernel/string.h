#pragma once
#include <stddef.h>

size_t strlen(const char* str);
char*  strcpy(char* dst, const char* src);
char*  strncpy(char* dst, const char* src, size_t num);
char*  strcat(char* dst, const char* src);
int    strcmp(const char* str1, const char* str2);
char*  strchr(const char* str, int character);
void   to_upper(char* str);
