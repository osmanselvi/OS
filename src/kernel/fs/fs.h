#pragma once
#include <stdint.h>
#include <stddef.h>

void fs_init();
int  fs_create_file(const char *name);
int  fs_find_file(const char *name);
void fs_write_file(const char *name, const char *text);
void fs_get_content(const char *name, char *buffer);
void get_file_list_string(char *buffer);
