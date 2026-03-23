#include "fs.h"
#include <drivers/ide.h>
#include <stdio.h>
#include <string.h>

#define MAX_FILES 32
#define FILE_NAME_LEN 32
#define FILE_CONTENT_LEN 2048
#define FS_LBA 1000
#define FS_MAGIC 0x514E434E // "SNCN"

typedef struct {
    uint32_t magic;
    char name[FILE_NAME_LEN];
    char content[FILE_CONTENT_LEN];
    int size;
    int used;
} FileEntry;

static FileEntry g_Files[MAX_FILES];

void fs_save() {
    // Write entire g_Files to disk (approx 66KB)
    // 32 files * ~2KB ≈ 64+KB. 1 sector = 512 bytes. 132 sectors.
    IDE_WriteSectors(FS_LBA, 132, g_Files);
}

void fs_load() {
    IDE_ReadSectors(FS_LBA, 132, g_Files);
    if (g_Files[0].magic != FS_MAGIC) {
        // First time initialization
        for (int i = 0; i < MAX_FILES; i++) {
            g_Files[i].used = 0;
            g_Files[i].magic = FS_MAGIC;
        }
        
        fs_create_file("benioku.txt");
        fs_write_file("benioku.txt", "SincanOs v2.0 - New World\n\nBu dosya artik DISK uzerinde tutulmaktadir!\nSistemi kapatsaniz da silinmez.\n\nMerhaba Dunya!");
        
        fs_create_file("notlar.txt");
        fs_write_file("notlar.txt", "Yapilacaklar:\n- Network Stack\n- USB Driver\n- Multi-tasking");
        
        fs_save();
    }
}

void fs_init() {
    fs_load();
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
            g_Files[i].magic = FS_MAGIC;
            fs_save();
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
        fs_save();
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

int fs_delete_file(const char *name) {
    int id = fs_find_file(name);
    if (id != -1) {
        g_Files[id].used = 0;
        fs_save();
        return 0;
    }
    return -1;
}

int fs_rename_file(const char *old_name, const char *new_name) {
    int id = fs_find_file(old_name);
    if (id != -1) {
        if (fs_find_file(new_name) != -1) return -1;
        strncpy(g_Files[id].name, new_name, FILE_NAME_LEN - 1);
        g_Files[id].name[FILE_NAME_LEN - 1] = '\0';
        fs_save();
        return 0;
    }
    return -1;
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
