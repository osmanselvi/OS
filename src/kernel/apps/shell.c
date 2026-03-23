#include "shell.h"
#include <gui/gui.h>
#include <drivers/keyboard.h>
#include <fs/fs.h>
#include <string.h>
#include <stdio.h>

#define SHELL_COLS 80
#define SHELL_ROWS 25
#define MAX_CMD_LEN 255

static char g_TerminalBuffer[SHELL_ROWS][SHELL_COLS];
static int g_CurRow = 0;
static int g_CurCol = 0;
static char g_InputBuffer[MAX_CMD_LEN];
static int g_InputLen = 0;

static void Shell_Clear() {
    for (int r = 0; r < SHELL_ROWS; r++) {
        for (int c = 0; c < SHELL_COLS; c++) {
            g_TerminalBuffer[r][c] = ' ';
        }
    }
    g_CurRow = 0;
    g_CurCol = 0;
}

static void Shell_Scroll() {
    for (int r = 0; r < SHELL_ROWS - 1; r++) {
        for (int c = 0; c < SHELL_COLS; c++) {
            g_TerminalBuffer[r][c] = g_TerminalBuffer[r + 1][c];
        }
    }
    for (int c = 0; c < SHELL_COLS; c++) {
        g_TerminalBuffer[SHELL_ROWS - 1][c] = ' ';
    }
    g_CurRow = SHELL_ROWS - 1;
}

static void Shell_Print(const char* s) {
    while (*s) {
        if (*s == '\n') {
            g_CurRow++;
            g_CurCol = 0;
        } else {
            g_TerminalBuffer[g_CurRow][g_CurCol++] = *s;
            if (g_CurCol >= SHELL_COLS) {
                g_CurRow++;
                g_CurCol = 0;
            }
        }
        if (g_CurRow >= SHELL_ROWS) {
            Shell_Scroll();
        }
        s++;
    }
}

static void Shell_PrintPrompt() {
    Shell_Print("sincan@OS > ");
}

static void Shell_ProcessCommand(const char* input) {
    char cmd_full[MAX_CMD_LEN];
    strcpy(cmd_full, input);
    
    char* cmd = cmd_full;
    while (*cmd == ' ') cmd++; // trim leading
    if (*cmd == '\0') { Shell_PrintPrompt(); return; }

    char* arg1 = strchr(cmd, ' ');
    if (arg1) {
        *arg1 = '\0';
        arg1++;
        while (*arg1 == ' ') arg1++;
    }

    char* arg2 = NULL;
    if (arg1) {
        arg2 = strchr(arg1, ' ');
        if (arg2) {
            *arg2 = '\0';
            arg2++;
            while (*arg2 == ' ') arg2++;
        }
    }

    char cmd_upper[32];
    strncpy(cmd_upper, cmd, 31);
    to_upper(cmd_upper);

    if (strcmp(cmd_upper, "HELP") == 0) {
        Shell_Print("Available commands: HELP, LS, CAT, READ, WRITE, TOUCH, RM, RENAME, MKDIR, TIME, CLEAR, EXIT\n");
    } else if (strcmp(cmd_upper, "LS") == 0) {
        char buf[512];
        get_file_list_string(buf);
        Shell_Print(buf);
    } else if (strcmp(cmd_upper, "CAT") == 0 || strcmp(cmd_upper, "READ") == 0) {
        if (arg1) {
            char buf[1024];
            fs_get_content(arg1, buf);
            if (buf[0] == '\0' && fs_find_file(arg1) == -1) Shell_Print("File not found.\n");
            else { Shell_Print(buf); Shell_Print("\n"); }
        } else Shell_Print("Usage: CAT <filename>\n");
    } else if (strcmp(cmd_upper, "TOUCH") == 0) {
        if (arg1) {
            if (fs_create_file(arg1) == -1) Shell_Print("Error: Could not create file.\n");
            else Shell_Print("File created.\n");
        } else Shell_Print("Usage: TOUCH <filename>\n");
    } else if (strcmp(cmd_upper, "RM") == 0) {
        if (arg1) {
            if (fs_delete_file(arg1) == -1) Shell_Print("Error: File not found.\n");
            else Shell_Print("File deleted.\n");
        } else Shell_Print("Usage: RM <filename>\n");
    } else if (strcmp(cmd_upper, "WRITE") == 0) {
        if (arg1 && arg2) {
            fs_write_file(arg1, arg2);
            Shell_Print("Done.\n");
        } else Shell_Print("Usage: WRITE <filename> <text>\n");
    } else if (strcmp(cmd_upper, "RENAME") == 0) {
        if (arg1 && arg2) {
            if (fs_rename_file(arg1, arg2) == -1) Shell_Print("Error: Rename failed. Ensure source exists and destination doesn't.\n");
            else Shell_Print("Renamed.\n");
        } else Shell_Print("Usage: RENAME <oldname> <newname>\n");
    } else if (strcmp(cmd_upper, "MKDIR") == 0) {
        if (arg1) {
            Shell_Print("Directory creation simulated: ");
            Shell_Print(arg1);
            Shell_Print("\n");
        } else Shell_Print("Usage: MKDIR <dirname>\n");
    } else if (strcmp(cmd_upper, "READ") == 0) {
        // Redundant due to CAT/READ above, but keeping for safety if needed
        if (arg1) {
            char buf[1024];
            fs_get_content(arg1, buf);
            if (buf[0] == '\0' && fs_find_file(arg1) == -1) Shell_Print("File not found.\n");
            else { Shell_Print(buf); Shell_Print("\n"); }
        } else Shell_Print("Usage: READ <filename>\n");
    } else if (strcmp(cmd_upper, "CLEAR") == 0) {
        Shell_Clear();
    } else if (strcmp(cmd_upper, "TIME") == 0) {
        Shell_Print("RTC Time: Placeholder (v2.0)\n");
    } else if (strcmp(cmd_upper, "EXIT") == 0) {
        Shell_Print("Please close the window to exit shell.\n");
    } else {
        Shell_Print("Unknown command: ");
        Shell_Print(cmd);
        Shell_Print("\n");
    }
    
    if (strcmp(cmd_upper, "CLEAR") != 0) Shell_PrintPrompt();
}

void Shell_Initialize() {
    Shell_Clear();
    Shell_Print("SincanOs Shell v1.2\n");
    Shell_PrintPrompt();
    g_InputLen = 0;
    g_InputBuffer[0] = '\0';
}

void Shell_Draw(int x, int y, int w, int h) {
    // Background
    GUI_FillRect(x, y, w, h, COL_BLACK);
    
    // Rows
    for (int r = 0; r < SHELL_ROWS; r++) {
        char line[SHELL_COLS + 1];
        for (int c = 0; c < SHELL_COLS; c++) {
            line[c] = g_TerminalBuffer[r][c];
        }
        line[SHELL_COLS] = '\0';
        GUI_DrawString(x + 2, y + 2 + r * FONT_H, line, COL_GREEN, COL_BLACK);
    }
    
    // Input line (current cursor)
    // Actually the current cursor is already in the buffer if we print properly.
    // Let's draw the current input buffer on the current line.
    int prompt_len = 12; // "sincan@OS > "
    char input_disp[MAX_CMD_LEN + 1];
    strcpy(input_disp, g_InputBuffer);
    // GUI_DrawString(x + 2 + prompt_len * FONT_W, y + 2 + g_CurRow * FONT_H, input_disp, COL_WHITE, COL_BLACK);
}

void Shell_HandleKey(char key) {
    if (key == '\n') {
        Shell_Print(g_InputBuffer);
        Shell_Print("\n");
        Shell_ProcessCommand(g_InputBuffer);
        g_InputLen = 0;
        g_InputBuffer[0] = '\0';
    } else if (key == '\b') {
        if (g_InputLen > 0) {
            g_InputLen--;
            g_InputBuffer[g_InputLen] = '\0';
            // We need to visually remove it from the screen if we were drawing it directly.
        }
    } else if (key >= 32 && key <= 126) {
        if (g_InputLen < MAX_CMD_LEN - 1) {
            g_InputBuffer[g_InputLen++] = key;
            g_InputBuffer[g_InputLen] = '\0';
        }
    }
}
