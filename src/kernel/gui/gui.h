#pragma once
#include <stdint.h>
#include <stdbool.h>

// ─── 32-bit RGB Color Palette ───────────────────────────────────────────────
#define COL_BLACK       0x00000000
#define COL_WHITE       0x00FFFFFF
#define COL_RED         0x00CC2211
#define COL_GREEN       0x0033BB44
#define COL_BLUE        0x001144CC
#define COL_YELLOW      0x00DDBB00
#define COL_CYAN        0x0000AACC
#define COL_ORANGE      0x00FF7700
#define COL_DARK_GREY   0x00444444
#define COL_MID_GREY    0x00888888
#define COL_LIGHT_GREY  0x00CCCCCC
#define COL_NEAR_WHITE  0x00EEEEEE
#define COL_TITLEBAR    0x00335577  /* navy-blue */
#define COL_TITLEBAR2   0x005588BB  /* lighter for gradient */
#define COL_DESKTOP     0x004488AA  /* teal desktop */
#define COL_TASKBAR     0x00E0E0E0
#define COL_TASKBAR_BTN 0x00CCDDEE
#define COL_CLOSE_BTN   0x00CC3322
#define COL_SHADOW      0x00222222

// ─── Drawing primitives ─────────────────────────────────────────────────────
void GUI_Initialize(uint32_t w, uint32_t h);
void GUI_SwapBuffers();

void GUI_PutPixel(int x, int y, uint32_t color);
void GUI_FillRect(int x, int y, int w, int h, uint32_t color);
void GUI_DrawRect(int x, int y, int w, int h, uint32_t color);     // outline only
void GUI_DrawLine(int x1, int y1, int x2, int y2, uint32_t color);
void GUI_DrawChar(int x, int y, unsigned char c, uint32_t fg, uint32_t bg);
void GUI_DrawString(int x, int y, const char *s, uint32_t fg, uint32_t bg);
void GUI_DrawStringTransparent(int x, int y, const char *s, uint32_t fg);
int  GUI_StringWidth(const char *s);

// ─── Window Manager ──────────────────────────────────────────────────────────
#define MAX_WINDOWS     10
#define MAX_ICONS       8
#define FONT_W          8
#define FONT_H          16

typedef struct {
    int  id;
    int  x, y, w, h;
    char title[48];
    bool active;
    bool dragging;
    int  drag_ox, drag_oy;
    char content[2048];
    char status[64];
    int  scroll_y, max_scroll_y;
} Window;

typedef struct {
    int  x, y;
    char label[24];
    char cmd[24];
} Icon;

void WM_Initialize();
void WM_CreateIcon(const char *label, const char *cmd, int x, int y);
void WM_CreateWindow(const char *title, int x, int y, int w, int h);
void WM_DrawAll();
int  WM_HandleMouse(int mx, int my, bool lbtn);
int  WM_HandleKey(char key);

// Return codes from WM_HandleMouse / WM_HandleKey
#define WM_CONTINUE  1
#define WM_EXIT      0
#define WM_SNAKE     2
#define WM_PONG      3
#define WM_NADIT     4
