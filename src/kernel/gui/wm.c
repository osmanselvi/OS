#include "gui.h"
#include <drivers/mouse.h>
#include <drivers/keyboard.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <apps/shell.h>

#define SCREEN_W    1024
#define SCREEN_H    768
#define TASKBAR_H   32
#define TITLEBAR_H  24
#define STATUS_H    20

// ─── State ───────────────────────────────────────────────────────────────────
static Window  g_Windows[MAX_WINDOWS];
static int     g_WinCount = 0;
static Icon    g_Icons[MAX_ICONS];
static int     g_IconCount = 0;

static bool  g_StartMenuOpen = false;
static int   g_DragWin = -1;
static int   g_DragOX, g_DragOY;
static int   g_FocusedWin = -1;

// ── itoa helper ──────────────────────────────────────────────────────────────
static void my_itoa(int n, char *buf) {
    if (n < 0) { *buf++ = '-'; n = -n; }
    char tmp[16]; int i = 0;
    if (n == 0) { buf[0]='0'; buf[1]=0; return; }
    while (n) { tmp[i++] = '0' + n%10; n /= 10; }
    for (int j = i-1; j >= 0; j--) *buf++ = tmp[j];
    *buf = 0;
}

// ─── Helpers ─────────────────────────────────────────────────────────────────
static void DrawButton3D(int x, int y, int w, int h, uint32_t face, bool pressed)
{
    GUI_FillRect(x, y, w, h, face);
    uint32_t hi = pressed ? COL_DARK_GREY  : COL_WHITE;
    uint32_t sh = pressed ? COL_WHITE      : COL_DARK_GREY;
    GUI_FillRect(x, y, w, 1, hi);      // top
    GUI_FillRect(x, y, 1, h, hi);      // left
    GUI_FillRect(x, y+h-1, w, 1, sh);  // bottom
    GUI_FillRect(x+w-1, y, 1, h, sh);  // right
}

// Titlebar gradient (two-tone approximation)
static void DrawTitlebar(int x, int y, int w, int h, bool focused)
{
    uint32_t top = focused ? COL_TITLEBAR2 : COL_DARK_GREY;
    uint32_t bot = focused ? COL_TITLEBAR  : 0x00555555;
    for (int i = 0; i < h; i++) {
        // Linear interpolation between top and bot
        uint32_t r = ((top >> 16 & 0xFF) * (h-i) + (bot >> 16 & 0xFF) * i) / h;
        uint32_t g2= ((top >>  8 & 0xFF) * (h-i) + (bot >>  8 & 0xFF) * i) / h;
        uint32_t b = ((top       & 0xFF) * (h-i) + (bot       & 0xFF) * i) / h;
        GUI_FillRect(x, y + i, w, 1, (r << 16) | (g2 << 8) | b);
    }
}

// ─── Window Manager Init ──────────────────────────────────────────────────────
void WM_Initialize()
{
    for (int i = 0; i < MAX_WINDOWS; i++) g_Windows[i].active = false;
    g_WinCount = 0; g_IconCount = 0;
    g_StartMenuOpen = false; g_DragWin = -1; g_FocusedWin = -1;
}

void WM_CreateIcon(const char *label, const char *cmd, int x, int y)
{
    if (g_IconCount >= MAX_ICONS) return;
    Icon *ic = &g_Icons[g_IconCount++];
    ic->x = x; ic->y = y;
    // strncpy safe copies
    int i = 0;
    while (label[i] && i < 23) { ic->label[i] = label[i]; i++; } ic->label[i] = 0;
    i = 0;
    while (cmd[i] && i < 23) { ic->cmd[i] = cmd[i]; i++; } ic->cmd[i] = 0;
}

void WM_CreateWindow(const char *title, int x, int y, int w, int h)
{
    if (g_WinCount >= MAX_WINDOWS) return;
    Window *win = &g_Windows[g_WinCount];
    win->id = g_WinCount++;
    win->x = x; win->y = y < TASKBAR_H ? TASKBAR_H : y;
    win->w = w; win->h = h;
    win->type = 0; // Normal
    win->active = true;
    win->dragging = false;
    win->scroll_y = 0; win->max_scroll_y = 0;
    win->content[0] = 0;
    // title copy
    int i = 0;
    while (title[i] && i < 47) { win->title[i] = title[i]; i++; } win->title[i] = 0;

    const char *status_msg = "Hazir";
    i = 0;
    while (status_msg[i] && i < 63) { win->status[i] = status_msg[i]; i++; } win->status[i] = 0;

    // Default content
    const char *default_content = "SincanOs New World\nTurkce karakter destegi: Merhaba!\nHazir.";
    i = 0;
    while (default_content[i] && i < 2047) { win->content[i] = default_content[i]; i++; }
    win->content[i] = 0;

    g_FocusedWin = win->id;
}

void WM_CreateShellWindow(const char *title, int x, int y, int w, int h)
{
    if (g_WinCount >= MAX_WINDOWS) return;
    Window *win = &g_Windows[g_WinCount];
    win->id = g_WinCount++;
    win->x = x; win->y = y < TASKBAR_H ? TASKBAR_H : y;
    win->w = w; win->h = h;
    win->type = 1; // Shell
    win->active = true;
    win->dragging = false;
    win->content[0] = 0;
    // title copy
    int i = 0;
    while (title[i] && i < 47) { win->title[i] = title[i]; i++; } win->title[i] = 0;

    const char *status_msg = "Hazir";
    i = 0;
    while (status_msg[i] && i < 63) { win->status[i] = status_msg[i]; i++; } win->status[i] = 0;

    Shell_Initialize();
    g_FocusedWin = win->id;
}

// ─── Draw Helpers ─────────────────────────────────────────────────────────────
static void DrawDesktopBackground()
{
    // Solid background (Teal) is much faster than per-row gradients
    GUI_FillRect(0, TASKBAR_H, SCREEN_W, SCREEN_H - TASKBAR_H, 0x000088AA);

    // Watermark bottom-right (Version Verification)
    GUI_DrawString(SCREEN_W - 160, SCREEN_H - 24, "SincanOs v2.0 - DEBUG P6", COL_WHITE, 0x000088AA);
    GUI_DrawString(SCREEN_W - 120, SCREEN_H - 8,  "Persistence Mode", 0x00AADDFF, 0x000088AA);
}

static void DrawIcons()
{
    for (int i = 0; i < g_IconCount; i++) {
        Icon *ic = &g_Icons[i];
        int x = ic->x, y = ic->y;

        // Icon body (48x48 rounded look)
        GUI_FillRect(x+2, y,   44,  2, COL_TITLEBAR2);
        GUI_FillRect(x,   y+2,  48, 44, COL_TITLEBAR2);
        GUI_FillRect(x+2, y+46, 44,  2, COL_TITLEBAR2);
        // Shine
        GUI_FillRect(x+4, y+4, 40, 6, COL_TITLEBAR);

        // Symbol inside: first letter of label
        char s[2] = {ic->label[0], 0};
        GUI_DrawString(x+20, y+18, s, COL_WHITE, 0xFFFFFFFF);

        // Label below
        int lw = GUI_StringWidth(ic->label);
        GUI_DrawString(x + (48 - lw)/2, y + 52, ic->label, COL_WHITE, 0xFFFFFFFF);
    }
}

static void DrawSingleWindow(Window *win)
{
    if (!win->active) return;
    int x = win->x, y = win->y, w = win->w, h = win->h;
    bool focused = (win->id == g_FocusedWin);

    // Shadow
    GUI_FillRect(x+4, y+4, w, h, 0x00111111);

    // Body
    GUI_FillRect(x, y, w, h, COL_NEAR_WHITE);

    // Content area
    GUI_FillRect(x+1, y+TITLEBAR_H, w-2, h-TITLEBAR_H-STATUS_H-2, (win->type == 1) ? COL_BLACK : COL_WHITE);

    if (win->type == 1) {
        Shell_Draw(x+1, y+TITLEBAR_H, w-2, h-TITLEBAR_H-STATUS_H-2);
        return;
    }

    // Content text
    int cx = x + 6, cy = y + TITLEBAR_H + 4;
    int clip_b = y + h - STATUS_H - 4;
    const char *p = win->content;
    char line[64]; int k = 0;
    int content_y = cy - win->scroll_y;
    while (*p) {
        if (*p == '\n') {
            line[k] = 0;
            if (content_y >= cy - FONT_H && content_y < clip_b)
                GUI_DrawString(cx, content_y, line, COL_BLACK, COL_WHITE);
            content_y += FONT_H + 2;
            k = 0;
        } else if (k < 63) {
            line[k++] = *p;
        }
        p++;
    }
    if (k > 0) {
        line[k] = 0;
        if (content_y >= cy - FONT_H && content_y < clip_b)
            GUI_DrawString(cx, content_y, line, COL_BLACK, COL_WHITE);
    }

    // Titlebar
    DrawTitlebar(x, y, w, TITLEBAR_H, focused);
    GUI_DrawString(x + 8, y + 4, win->title, COL_WHITE, 0xFFFFFFFF);

    // Close button
    int close_x = x + w - 22;
    GUI_FillRect(close_x, y + 3, 18, 18, COL_CLOSE_BTN);
    GUI_FillRect(close_x+1, y+4, 16, 1, 0x00FF7766);
    GUI_DrawString(close_x + 5, y + 4, "X", COL_WHITE, 0xFFFFFFFF);

    // Status bar
    GUI_FillRect(x, y + h - STATUS_H, w, STATUS_H, 0x00DDDDDD);
    GUI_FillRect(x, y + h - STATUS_H, w, 1, COL_MID_GREY);
    GUI_DrawString(x + 4, y + h - STATUS_H + 2, win->status, COL_DARK_GREY, 0x00DDDDDD);

    // Border
    GUI_DrawRect(x, y, w, h, focused ? COL_TITLEBAR : COL_DARK_GREY);
}

static void DrawTaskbar()
{
    // Taskbar background
    GUI_FillRect(0, 0, SCREEN_W, TASKBAR_H, COL_TASKBAR);
    GUI_FillRect(0, TASKBAR_H-1, SCREEN_W, 1, COL_MID_GREY);
    GUI_FillRect(0, TASKBAR_H-2, SCREEN_W, 1, COL_WHITE);

    // Start button
    DrawButton3D(4, 3, 90, TASKBAR_H-6, COL_TASKBAR_BTN, g_StartMenuOpen);
    // Green icon
    GUI_FillRect(10, 10, 8, 8, COL_GREEN);
    GUI_FillRect(11, 11, 6, 6, 0x0077FF77);
    GUI_DrawString(22, 8, "BASLAT", COL_BLACK, 0xFFFFFFFF);

    // Window buttons (mini)
    int bx = 100;
    for (int i = 0; i < g_WinCount; i++) {
        if (!g_Windows[i].active) continue;
        bool focused = (g_Windows[i].id == g_FocusedWin);
        DrawButton3D(bx, 4, 120, TASKBAR_H-8, focused ? COL_TITLEBAR : COL_TASKBAR_BTN, focused);
        // Title (truncated)
        char short_title[14];
        int ti = 0;
        while (g_Windows[i].title[ti] && ti < 13) { short_title[ti] = g_Windows[i].title[ti]; ti++; }
        short_title[ti] = 0;
        GUI_DrawString(bx+4, 9, short_title, focused ? COL_WHITE : COL_BLACK, 0xFFFFFFFF);
        bx += 126;
        if (bx > SCREEN_W - 200) break;
    }

    // Clock (right side) - static placeholder
    GUI_FillRect(SCREEN_W - 80, 4, 76, TASKBAR_H-8, 0x00DDEEFF);
    GUI_DrawRect(SCREEN_W - 80, 4, 76, TASKBAR_H-8, COL_MID_GREY);
    GUI_DrawString(SCREEN_W - 76, 9, "SincanOS", COL_TITLEBAR, 0x00DDEEFF);
}

static void DrawStartMenu()
{
    if (!g_StartMenuOpen) return;
    int mx = 4, my = TASKBAR_H, mw = 180, mh = 200;

    // Shadow
    GUI_FillRect(mx+4, my+4, mw, mh, COL_SHADOW);
    // Body
    GUI_FillRect(mx, my, mw, mh, COL_NEAR_WHITE);
    // Side stripe
    GUI_FillRect(mx, my, 20, mh, COL_TITLEBAR);
    // Stripe text
    GUI_DrawString(mx+4, my+mh-80, "S", COL_NEAR_WHITE, 0xFFFFFFFF);
    GUI_DrawString(mx+4, my+mh-64, "O", COL_NEAR_WHITE, 0xFFFFFFFF);
    GUI_DrawString(mx+4, my+mh-48, "S", COL_NEAR_WHITE, 0xFFFFFFFF);
    GUI_DrawRect(mx, my, mw, mh, COL_DARK_GREY);

    int ix = mx + 28, iy = my + 8;
    GUI_DrawString(ix, iy, "PROGRAMLAR", COL_DARK_GREY, COL_NEAR_WHITE); iy += 20;
    GUI_DrawString(ix+4, iy, "Not Defteri",   COL_BLACK, COL_NEAR_WHITE); iy += 18;
    GUI_DrawString(ix+4, iy, "Dosya Yoneticisi", COL_BLACK, COL_NEAR_WHITE); iy += 18;
    GUI_DrawString(ix+4, iy, "Terminal",      COL_BLACK, COL_NEAR_WHITE); iy += 24;
    GUI_FillRect(ix, iy-2, mw-36, 1, COL_MID_GREY); 
    GUI_DrawString(ix, iy, "OYUNLAR", COL_DARK_GREY, COL_NEAR_WHITE); iy += 20;
    GUI_DrawString(ix+4, iy, "Snake", 0x0033BB33, COL_NEAR_WHITE); iy += 18;
    GUI_DrawString(ix+4, iy, "Pong",  0x001144CC, COL_NEAR_WHITE); iy += 24;
    GUI_FillRect(ix, iy-2, mw-36, 1, COL_MID_GREY);
    GUI_DrawString(ix, iy, "SISTEM", COL_DARK_GREY, COL_NEAR_WHITE); iy += 20;
    GUI_DrawString(ix+4, iy, "Kapat", COL_RED, COL_NEAR_WHITE);
}

static void DrawMouseCursor(int mx, int my)
{
    // Simple arrow cursor
    static const unsigned char cursor[12][12] = {
        {1,0,0,0,0,0,0,0,0,0,0,0},
        {1,2,0,0,0,0,0,0,0,0,0,0},
        {1,2,2,0,0,0,0,0,0,0,0,0},
        {1,2,2,2,0,0,0,0,0,0,0,0},
        {1,2,2,2,2,0,0,0,0,0,0,0},
        {1,2,2,2,2,2,0,0,0,0,0,0},
        {1,2,2,2,2,2,2,0,0,0,0,0},
        {1,2,2,2,1,1,0,0,0,0,0,0},
        {1,2,1,0,1,2,0,0,0,0,0,0},
        {1,1,0,0,0,1,2,0,0,0,0,0},
        {0,0,0,0,0,1,2,0,0,0,0,0},
        {0,0,0,0,0,0,1,0,0,0,0,0},
    };
    for (int cy = 0; cy < 12; cy++)
        for (int cx2 = 0; cx2 < 12; cx2++) {
            unsigned char v = cursor[cy][cx2];
            if (v == 1) GUI_PutPixel(mx+cx2, my+cy, COL_BLACK);
            else if (v == 2) GUI_PutPixel(mx+cx2, my+cy, COL_WHITE);
        }
}

// ─── Public API ───────────────────────────────────────────────────────────────
void WM_DrawAll()
{
    DrawDesktopBackground();
    DrawIcons();
    for (int i = 0; i < g_WinCount; i++)
        DrawSingleWindow(&g_Windows[i]);
    DrawTaskbar();
    DrawStartMenu();

    // Draw cursor on top
    MouseState ms = Mouse_GetState();
    DrawMouseCursor(ms.x, ms.y);
}

// Returns 1 to keep running, 0 to exit
int WM_HandleMouse(int mx, int my, bool lbtn)
{
    static bool prev_lbtn = false;
    bool clicked  = lbtn && !prev_lbtn;
    bool released = !lbtn && prev_lbtn;

    // Start button click
    if (clicked && mx >= 4 && mx <= 94 && my >= 3 && my <= TASKBAR_H-3) {
        g_StartMenuOpen = !g_StartMenuOpen;
        prev_lbtn = lbtn;
        return 1;
    }

    // Start Menu interaction
    if (g_StartMenuOpen && clicked) {
        int smx = 4, smy = TASKBAR_H;
        if (mx >= smx && mx <= smx+180 && my >= smy && my <= smy+200) {
            // Menu items (match the y-positions from DrawStartMenu)
            // iy starts at smy+8, PROGRAMLAR header (+20), then:
            int iy = smy + 8 + 20;  // first item: Not Defteri
            if (my >= iy && my < iy+18) { g_StartMenuOpen=false; prev_lbtn=lbtn; return WM_NADIT; } iy += 18;
            if (my >= iy && my < iy+18) { WM_CreateWindow("Dosya Yoneticisi", 200, 120, 400, 300); g_StartMenuOpen=false; WM_DrawAll(); prev_lbtn=lbtn; return WM_CONTINUE; } iy += 18;
            if (my >= iy && my < iy+18) { WM_CreateShellWindow("Komut Satiri", 150, 100, 640, 420); g_StartMenuOpen=false; WM_DrawAll(); prev_lbtn=lbtn; return WM_CONTINUE; } iy += 24; // +24 includes separator gap
            // OYUNLAR header (+20)
            iy += 20;
            if (my >= iy && my < iy+18) { g_StartMenuOpen=false; prev_lbtn=lbtn; return WM_SNAKE; } iy += 18;
            if (my >= iy && my < iy+18) { g_StartMenuOpen=false; prev_lbtn=lbtn; return WM_PONG;  } iy += 24;
            // SISTEM header (+20)
            iy += 20;
            if (my >= iy && my < iy+18) { prev_lbtn=lbtn; return WM_EXIT; } // Kapat
            g_StartMenuOpen = false;
        } else {
            g_StartMenuOpen = false;
        }
        prev_lbtn = lbtn;
        return WM_CONTINUE;
    }

    // Window interactions
    if (lbtn && g_DragWin != -1) {
        // Dragging
        Window *win = &g_Windows[g_DragWin];
        int nx = mx - g_DragOX;
        int ny = my - g_DragOY;
        if (ny < TASKBAR_H) ny = TASKBAR_H;
        win->x = nx; win->y = ny;
        prev_lbtn = lbtn;
        return 1;
    }

    if (released && g_DragWin != -1) {
        g_DragWin = -1;
    }

    if (clicked) {
        // Check windows top-to-bottom
        for (int i = g_WinCount-1; i >= 0; i--) {
            Window *win = &g_Windows[i];
            if (!win->active) continue;
            if (mx >= win->x && mx <= win->x+win->w && my >= win->y && my <= win->y+win->h) {
                g_FocusedWin = win->id;
                // Close button
                if (mx >= win->x+win->w-22 && mx <= win->x+win->w-4 && my >= win->y+3 && my <= win->y+21) {
                    win->active = false;
                    if (g_FocusedWin == win->id) g_FocusedWin = -1;
                    prev_lbtn = lbtn;
                    return 1;
                }
                // Titlebar drag
                if (my >= win->y && my <= win->y + TITLEBAR_H) {
                    g_DragWin = i;
                    g_DragOX = mx - win->x;
                    g_DragOY = my - win->y;
                }
                prev_lbtn = lbtn;
                return 1;
            }
        }

        // Desktop icon click
        for (int i = 0; i < g_IconCount; i++) {
            if (mx >= g_Icons[i].x && mx <= g_Icons[i].x+48 &&
                my >= g_Icons[i].y && my <= g_Icons[i].y+48) {
                if (strcmp(g_Icons[i].cmd, "SHELL") == 0) {
                    WM_CreateShellWindow("Komut Satiri", 150, 100, 640, 420);
                } else {
                    WM_CreateWindow(g_Icons[i].label, 100+i*30, 100+i*20, 400, 280);
                }
                break;
            }
        }
    }

    prev_lbtn = lbtn;
    return 1;
}

int WM_HandleKey(char key)
{
    if (g_FocusedWin < 0 || g_FocusedWin >= g_WinCount) return 1;
    Window *win = &g_Windows[g_FocusedWin];
    if (!win->active) return 1;

    if (win->type == 1) {
        Shell_HandleKey(key);
        return 1;
    }

    uint16_t len = 0;
    while (win->content[len] && len < 2046) len++;

    if (key == '\b') {
        if (len > 0) win->content[len-1] = 0;
    } else if (key == '\r' || key == '\n') {
        if (len < 2046) { win->content[len] = '\n'; win->content[len+1] = 0; }
    } else if (key >= 32 && len < 2046) {
        win->content[len] = key; win->content[len+1] = 0;
    }

    // Redraw removed, handled by main loop
    return 1;
}
