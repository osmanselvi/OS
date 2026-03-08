#include "fs.h"
#include <stdio.h>
#include <string.h>

#define MAX_FILES 32
#define FILE_NAME_LEN 32
#define FILE_CONTENT_LEN 2048

typedef struct {
    char name[FILE_NAME_LEN];
    char content[FILE_CONTENT_LEN];
    int size;
    int used;
} FileEntry;

static FileEntry g_Files[MAX_FILES];

void fs_init() {
    for (int i = 0; i < MAX_FILES; i++) {
        g_Files[i].used = 0;
    }
    
    // Default Files
    fs_create_file("benioku.txt");
    fs_write_file("benioku.txt", "SincanOs v2.0 - New World\n\nBu dosya RAM uzerinde tutulmaktadir.\nNadit editoru ile duzenleyebilirsiniz.\n\nMerhaba Dunya!");
    
    fs_create_file("notlar.txt");
    fs_write_file("notlar.txt", "Yapilacaklar:\n- FS Disk Destegi\n- Network Stack\n- USB Driver");
}

int fs_create_file(const char *name) {
    if (fs_find_file(name) != -1) return -1;
    for (int i = 0; i < MAX_FILES; i++) {
        if (!g_Files[i].used) {
            strncpy(g_Files[i].name, name, FILE_NAME_LEN - 1);
            g_Files[i].name[FILE_NAME_LEN - 1] = '\0';
            g_Files[i].used = 1;
            g_Files[i].size = 0;
            g_Files[i].content[0] = '\0';
            return i;
        }
    }
    return -1;
}

int fs_find_file(const char *name) {
    for (int i = 0; i < MAX_FILES; i++) {
        if (g_Files[i].used && strcmp(g_Files[i].name, name) == 0) {
            return i;
        }
    }
    return -1;
}

void fs_write_file(const char *name, const char *text) {
    int id = fs_find_file(name);
    if (id != -1) {
        strncpy(g_Files[id].content, text, FILE_CONTENT_LEN - 1);
        g_Files[id].content[FILE_CONTENT_LEN - 1] = '\0';
        g_Files[id].size = strlen(g_Files[id].content);
    }
}

void fs_get_content(const char *name, char *buffer) {
    int id = fs_find_file(name);
    if (id != -1) {
        strcpy(buffer, g_Files[id].content);
    } else {
        buffer[0] = '\0';
    }
}

void get_file_list_string(char *buffer) {
    buffer[0] = '\0';
    for (int i = 0; i < MAX_FILES; i++) {
        if (g_Files[i].used) {
            strcat(buffer, g_Files[i].name);
            strcat(buffer, "\n");
        }
    }
}
