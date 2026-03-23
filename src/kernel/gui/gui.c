#include "gui.h"
#include "font.h"
#include <arch/i686/vesa.h>
#include <memory.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

static uint32_t g_W, g_H;
static uint32_t* g_BackBuffer = NULL;

void GUI_Initialize(uint32_t w, uint32_t h)
{
    g_W = w; g_H = h;
    if (g_BackBuffer) kfree(g_BackBuffer);
    g_BackBuffer = (uint32_t*)kmalloc(w * h * 4);
    if (!g_BackBuffer) {
        printf("GUI: Failed to allocate backbuffer!\n");
        for(;;);
    }
    memset(g_BackBuffer, 0, w * h * 4);
}

void GUI_SwapBuffers()
{
    if (g_BackBuffer) VESA_Blit(g_BackBuffer);
}

void GUI_PutPixel(int x, int y, uint32_t color)
{
    if (x < 0 || y < 0 || (uint32_t)x >= g_W || (uint32_t)y >= g_H || !g_BackBuffer) return;
    g_BackBuffer[y * g_W + x] = color;
}

void GUI_FillRect(int x, int y, int w, int h, uint32_t color)
{
    if (w <= 0 || h <= 0 || !g_BackBuffer) return;
    
    int x1 = x < 0 ? 0 : x;
    int y1 = y < 0 ? 0 : y;
    int x2 = x + w; if ((uint32_t)x2 > g_W) x2 = (int)g_W;
    int y2 = y + h; if ((uint32_t)y2 > g_H) y2 = (int)g_H;

    if (x2 <= x1 || y2 <= y1) return;

    for (int row = y1; row < y2; row++) {
        uint32_t* p = &g_BackBuffer[row * g_W + x1];
        for (int col = x1; col < x2; col++) {
            *p++ = color;
        }
    }
}

void GUI_DrawRect(int x, int y, int w, int h, uint32_t color)
{
    GUI_FillRect(x, y, w, 1, color);
    GUI_FillRect(x, y+h-1, w, 1, color);
    GUI_FillRect(x, y, 1, h, color);
    GUI_FillRect(x+w-1, y, 1, h, color);
}

void GUI_DrawLine(int x1, int y1, int x2, int y2, uint32_t color)
{
    int dx = x2 - x1; int dy = y2 - y1;
    int abs_dx = dx < 0 ? -dx : dx;
    int abs_dy = dy < 0 ? -dy : dy;
    int steps = abs_dx > abs_dy ? abs_dx : abs_dy;
    if (!steps) { GUI_PutPixel(x1, y1, color); return; }
    for (int s = 0; s <= steps; s++)
        GUI_PutPixel(x1 + dx * s / steps, y1 + dy * s / steps, color);
}

void GUI_DrawChar(int x, int y, unsigned char c, uint32_t fg, uint32_t bg)
{
    if (!g_BackBuffer || x < 0 || y < 0 || (uint32_t)x + FONT_W > g_W || (uint32_t)y + FONT_H > g_H) return;
    const unsigned char* glyph = g_Font8x16[c];
    for (int row = 0; row < FONT_H; row++) {
        unsigned char bits = glyph[row];
        uint32_t* p = &g_BackBuffer[(y + row) * g_W + x];
        for (int col = 0; col < FONT_W; col++) {
            if (bits & (0x80 >> col))
                p[col] = fg;
            else if (bg != 0xFFFFFFFF)
                p[col] = bg;
        }
    }
}

void GUI_DrawString(int x, int y, const char *s, uint32_t fg, uint32_t bg)
{
    int cx = x;
    while (*s) { GUI_DrawChar(cx, y, (unsigned char)*s, fg, bg); cx += FONT_W; s++; }
}

void GUI_DrawStringTransparent(int x, int y, const char *s, uint32_t fg)
{
    GUI_DrawString(x, y, s, fg, 0xFFFFFFFF);
}

int GUI_StringWidth(const char *s)
{
    int n = 0; while (*s++) n++;
    return n * FONT_W;
}
