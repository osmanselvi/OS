#include "nadit.h"
#include <gui/gui.h>
#include <arch/i686/vesa.h>
#include <drivers/keyboard.h>
#include <fs/fs.h>
#include <memory.h>
#include <string.h>
#include <stdbool.h>

#define TAB_SIZE 4
#define MARGIN_T 32
#define MARGIN_B 32
#define MARGIN_L 8

#define COL_BG       0x001E1E1E
#define COL_TEXT     0x00D4D4D4
#define COL_GUTTER   0x002D2D2D
#define COL_STATUS   0x00007ACC
#define COL_STATUS_T 0x00FFFFFF
#define COL_CURSOR   COL_YELLOW

typedef struct erow {
    int size;
    int rsize;
    char *chars;
    char *render;
    struct erow *prev;
    struct erow *next;
} erow;

struct {
    int cx, cy;
    int rowoff;
    int numrows;
    erow *head;
    char filename[32];
    bool dirty;
    char status[80];
} E;

static void update_row(erow *row) {
    int tabs = 0;
    for (int j = 0; j < row->size; j++) if (row->chars[j] == '\t') tabs++;
    if (row->render) kfree(row->render);
    row->render = kmalloc(row->size + tabs * (TAB_SIZE - 1) + 1);
    int idx = 0;
    for (int j = 0; j < row->size; j++) {
        if (row->chars[j] == '\t') {
            row->render[idx++] = ' ';
            while (idx % TAB_SIZE != 0) row->render[idx++] = ' ';
        } else row->render[idx++] = row->chars[j];
    }
    row->render[idx] = '\0';
    row->rsize = idx;
}

static void insert_row(int at, const char *s, size_t len) {
    erow *new_row = kmalloc(sizeof(erow));
    new_row->size = len;
    new_row->chars = kmalloc(len + 1);
    memcpy(new_row->chars, s, len);
    new_row->chars[len] = '\0';
    new_row->render = NULL;
    update_row(new_row);

    if (!E.head) {
        new_row->prev = new_row->next = NULL;
        E.head = new_row;
    } else {
        erow *curr = E.head;
        int i = 0;
        while (i < at && curr->next) { curr = curr->next; i++; }
        if (at >= E.numrows) { // Append
            curr->next = new_row; new_row->prev = curr; new_row->next = NULL;
        } else if (at == 0) { // Prepended
            new_row->next = E.head; new_row->prev = NULL; E.head->prev = new_row; E.head = new_row;
        } else { // Insert before
            new_row->prev = curr->prev; new_row->next = curr;
            curr->prev->next = new_row; curr->prev = new_row;
        }
    }
    E.numrows++;
}

static void free_rows() {
    erow *row = E.head;
    while (row) {
        erow *next = row->next;
        kfree(row->render); kfree(row->chars); kfree(row);
        row = next;
    }
    E.head = NULL; E.numrows = 0;
}

static void editor_open(const char *filename) {
    if (!filename) { strcpy(E.filename, "Adsiz"); return; }
    strncpy(E.filename, filename, 31);
    char *buf = kmalloc(2048);
    fs_get_content(filename, buf);
    char *p = buf, *start = buf;
    while (*p) {
        if (*p == '\n') { insert_row(E.numrows, start, p - start); start = p + 1; }
        p++;
    }
    if (p > start) insert_row(E.numrows, start, p - start);
    kfree(buf);
    E.dirty = false;
}

static void editor_save() {
    int tot = 0; erow *r = E.head;
    while (r) { tot += r->size + 1; r = r->next; }
    char *buf = kmalloc(tot + 1); char *p = buf;
    r = E.head;
    while (r) { memcpy(p, r->chars, r->size); p += r->size; *p++ = '\n'; r = r->next; }
    *p = '\0';
    if (fs_find_file(E.filename) == -1) fs_create_file(E.filename);
    fs_write_file(E.filename, buf);
    kfree(buf); E.dirty = false;
    strcpy(E.status, "Kaydedildi.");
}

static void refresh_screen() {
    int rows = (768 - MARGIN_T - MARGIN_B) / 16;
    if (E.cy < E.rowoff) E.rowoff = E.cy;
    if (E.cy >= E.rowoff + rows) E.rowoff = E.cy - rows + 1;

    VESA_ClearScreen(COL_BG);
    GUI_FillRect(0, 0, 1024, MARGIN_T, COL_GUTTER);
    GUI_DrawString(16, 8, "NADIT Editor v2.0", COL_STATUS_T, COL_GUTTER);
    GUI_DrawString(200, 8, E.filename, 0x00FFFF00, COL_GUTTER);

    erow *r = E.head;
    for (int i = 0; i < E.rowoff && r; i++) r = r->next;
    for (int y = 0; y < rows; y++) {
        int py = MARGIN_T + y * 16;
        if (r) {
            GUI_DrawString(MARGIN_L, py, r->render, COL_TEXT, COL_BG);
            if (E.cy == E.rowoff + y) { // Cursor
                GUI_FillRect(MARGIN_L + E.cx * 8, py, 2, 16, COL_CURSOR);
            }
            r = r->next;
        } else GUI_DrawString(MARGIN_L, py, "~", 0x00555555, COL_BG);
    }

    GUI_FillRect(0, 768 - MARGIN_B, 1024, MARGIN_B, COL_STATUS);
    GUI_DrawString(16, 768 - 24, "Ctrl-S: Kaydet | Q: Cikis | ", COL_STATUS_T, COL_STATUS);
    GUI_DrawString(300, 768 - 24, E.status, COL_STATUS_T, COL_STATUS);
}

void Nadit_Run(const char *filename) {
    memset(&E, 0, sizeof(E));
    editor_open(filename);
    while (1) {
        refresh_screen();
        uint8_t sc = Keyboard_GetLastScancode();
        char ch = Keyboard_GetLastChar();
        if (sc == 0x01 || ch == 'q' || ch == 'Q') break;
        if (ch == 19) editor_save(); // Ctrl-S (simplistic)
        if (sc == 0x48 && E.cy > 0) E.cy--; // Up
        if (sc == 0x50 && E.cy < E.numrows - 1) E.cy++; // Down
        if (sc == 0x4B && E.cx > 0) E.cx--; // Left
        if (sc == 0x4D) { // Right
            erow *curr = E.head; for(int i=0; i<E.cy; i++) curr = curr->next;
            if (curr && E.cx < curr->size) E.cx++;
        }
        for(volatile int k=0; k<1000000; k++);
    }
    free_rows();
}
