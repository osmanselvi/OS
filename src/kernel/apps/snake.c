#include "snake.h"
#include <gui/gui.h>
#include <arch/i686/vesa.h>
#include <drivers/keyboard.h>
#include <stdbool.h>
#include <stdint.h>

// ─── Grid & Rendering Config ─────────────────────────────────────────────────
#define CELL      16          // pixels per grid cell
#define COLS      48          // 48 * 16 = 768px wide
#define ROWS      36          // 36 * 16 = 576px tall
#define OFF_X     128         // center horizontally (128 + 768 = 896, centered in 1024)
#define OFF_Y      96         // leave space for score bar

#define COL_BG        0x00102820  // dark green-black
#define COL_BORDER    0x0033AA33
#define COL_HEAD      0x0044FF44
#define COL_BODY      0x0022BB22
#define COL_FOOD      0x00FF4422
#define COL_SCORE_BG  0x00111111
#define COL_TEXT      0x00FFFFFF

typedef struct { int x, y; } Pt;

static unsigned long g_rng = 12345;
static int s_rand(void) {
    g_rng = g_rng * 1103515245 + 12345;
    return (int)((g_rng >> 16) & 0x7FFF);
}

static void draw_cell(int x, int y, uint32_t col) {
    GUI_FillRect(OFF_X + x*CELL+1, OFF_Y + y*CELL+1, CELL-2, CELL-2, col);
}

static void draw_num(int x, int y, int n, uint32_t fg, uint32_t bg)
{
    char buf[12]; int i = 0;
    if (n == 0) { buf[i++] = '0'; }
    else {
        int tmp = n;
        char rev[12]; int r = 0;
        while(tmp) { rev[r++] = '0' + tmp%10; tmp /= 10; }
        while(r--) buf[i++] = rev[r+0]; // note: r is decremented already, use r+1
    }
    buf[i] = 0;
    // Simple: rebuild properly
    {
        char b2[12]; int j = 0, tmp2 = n;
        if(tmp2 == 0) { b2[j++] = '0'; }
        else {
            char r2[12]; int rlen = 0;
            while(tmp2) { r2[rlen++] = '0' + tmp2%10; tmp2 /= 10; }
            for(int k = rlen-1; k >= 0; k--) b2[j++] = r2[k];
        }
        b2[j] = 0;
        GUI_DrawString(x, y, b2, fg, bg);
    }
}

void Snake_Run()
{
    // ── Draw arena ──
    VESA_ClearScreen(COL_BG);
    // Score bar
    GUI_FillRect(0, 0, 1024, OFF_Y, COL_SCORE_BG);
    GUI_DrawString(OFF_X, 20, "SNAKE  -  W/A/S/D  veya  Ok Tuslari  |  Q: Cikis", COL_TEXT, COL_SCORE_BG);
    // Border
    for(int x = -1; x <= COLS; x++) {
        GUI_FillRect(OFF_X + x*CELL, OFF_Y - CELL, CELL, CELL, COL_BORDER); // top
        GUI_FillRect(OFF_X + x*CELL, OFF_Y + ROWS*CELL, CELL, CELL, COL_BORDER); // bottom
    }
    for(int y = -1; y <= ROWS; y++) {
        GUI_FillRect(OFF_X - CELL, OFF_Y + y*CELL, CELL, CELL, COL_BORDER); // left
        GUI_FillRect(OFF_X + COLS*CELL, OFF_Y + y*CELL, CELL, CELL, COL_BORDER); // right
    }

    // ── Snake setup ──
    Pt snake[200]; int len = 4;
    snake[0] = (Pt){COLS/2, ROWS/2};
    snake[1] = (Pt){COLS/2-1, ROWS/2};
    snake[2] = (Pt){COLS/2-2, ROWS/2};
    snake[3] = (Pt){COLS/2-3, ROWS/2};

    Pt food = {s_rand() % COLS, s_rand() % ROWS};

    int dx = 1, dy = 0;
    int score = 0;
    bool game_over = false;

    // Draw initial state
    for(int i = 1; i < len; i++) draw_cell(snake[i].x, snake[i].y, COL_BODY);
    draw_cell(snake[0].x, snake[0].y, COL_HEAD);
    draw_cell(food.x, food.y, COL_FOOD);

    Keyboard_GetLastChar(); // flush

    while (!game_over) {
        // Input
        uint8_t sc = Keyboard_GetLastScancode();
        char ch  = Keyboard_GetLastChar();

        if ((sc == 0x48 || ch == 'w' || ch == 'W') && dy != 1)  { dx = 0; dy = -1; }
        if ((sc == 0x50 || ch == 's' || ch == 'S') && dy != -1) { dx = 0; dy =  1; }
        if ((sc == 0x4B || ch == 'a' || ch == 'A') && dx != 1)  { dx = -1; dy = 0; }
        if ((sc == 0x4D || ch == 'd' || ch == 'D') && dx != -1) { dx =  1; dy = 0; }
        if (sc == 0x01 || ch == 'q' || ch == 'Q')               { game_over = true; break; }

        // Erase tail
        Pt tail = snake[len-1];
        draw_cell(tail.x, tail.y, COL_BG);

        // Move
        for(int i = len-1; i > 0; i--) snake[i] = snake[i-1];
        snake[0].x += dx;
        snake[0].y += dy;

        // Wall collision
        if (snake[0].x < 0 || snake[0].x >= COLS || snake[0].y < 0 || snake[0].y >= ROWS)
            game_over = true;

        // Self collision
        for(int i = 1; i < len; i++)
            if (snake[0].x == snake[i].x && snake[0].y == snake[i].y)
                game_over = true;

        if (game_over) break;

        // Eat
        if (snake[0].x == food.x && snake[0].y == food.y) {
            score += 10;
            snake[len++] = tail;
            do { food = (Pt){s_rand() % COLS, s_rand() % ROWS}; } while(0);
            draw_cell(food.x, food.y, COL_FOOD);
        }

        // Draw head & neck
        draw_cell(snake[1].x, snake[1].y, COL_BODY);
        draw_cell(snake[0].x, snake[0].y, COL_HEAD);

        // Score
        GUI_FillRect(800, 16, 200, 24, COL_SCORE_BG);
        GUI_DrawString(800, 16, "Skor: ", COL_TEXT, COL_SCORE_BG);
        draw_num(862, 16, score, 0x00FFFF00, COL_SCORE_BG);

        // Speed delay
        for(volatile int k = 0; k < 2000000; k++);
    }

    // Game over screen
    GUI_FillRect(350, 300, 324, 120, 0x00220000);
    GUI_DrawRect(350, 300, 324, 120, COL_FOOD);
    GUI_DrawString(390, 320, "GAME OVER!", 0x00FF4444, 0x00220000);
    GUI_DrawString(368, 344, "Final Skor: ", COL_TEXT, 0x00220000);
    draw_num(490, 344, score, 0x00FFFF00, 0x00220000);
    GUI_DrawString(376, 376, "Devam icin ENTER...", COL_TEXT, 0x00220000);

    // Wait for ENTER
    while(1) {
        uint8_t sc = Keyboard_GetLastScancode();
        char ch = Keyboard_GetLastChar();
        if (sc == 0x1C || ch == '\r' || ch == '\n' || ch == 'q' || ch == 'Q') break;
        for(volatile int k = 0; k < 100000; k++);
    }
}
