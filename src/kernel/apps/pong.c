#include "pong.h"
#include <gui/gui.h>
#include <arch/i686/vesa.h>
#include <drivers/keyboard.h>
#include <stdbool.h>
#include <stdint.h>

// ─── Pong Config (scaled up for 1024x768) ────────────────────────────────────
#define PW      12      // paddle width
#define PH      80      // paddle height
#define BS      10      // ball size
#define SW      1024
#define SH      768
#define P1_X    30
#define P2_X    (SW - P1_X - PW)

#define COL_BG      0x00000820
#define COL_PADDLE  0x00FFFFFF
#define COL_BALL    0x00FFEE22
#define COL_MID     0x00223344
#define COL_SCORE   0x00AACCEE
#define COL_UI      0x00FFFFFF

static void draw_num2(int x, int y, int n) {
    char b[8]; int i = 0, tmp = n;
    if(tmp == 0) { b[i++] = '0'; }
    else {
        char r[8]; int rl = 0;
        while(tmp) { r[rl++] = '0' + tmp%10; tmp /= 10; }
        for(int k = rl-1; k >= 0; k--) b[i++] = r[k];
    }
    b[i] = 0;
    GUI_DrawString(x, y, b, COL_SCORE, COL_BG);
}

void Pong_Run()
{
    VESA_ClearScreen(COL_BG);

    // Center dashed line
    for(int y = 0; y < SH; y += 20)
        GUI_FillRect(SW/2 - 2, y, 4, 10, COL_MID);

    GUI_DrawString(380, 16, "P1: W/S    P2: O/L    ESC: Cikis", COL_UI, COL_BG);

    int p1y = SH/2 - PH/2;
    int p2y = SH/2 - PH/2;
    int bx  = SW/2 - BS/2;
    int by  = SH/2 - BS/2;
    int bdx = 3, bdy = 2;
    int s1 = 0, s2 = 0;
    bool running = true;
    int speed_cnt = 0;

    // Initial draw
    GUI_FillRect(P1_X, p1y, PW, PH, COL_PADDLE);
    GUI_FillRect(P2_X, p2y, PW, PH, COL_PADDLE);
    GUI_FillRect(bx, by, BS, BS, COL_BALL);

    while (running) {
        // Erase old positions
        GUI_FillRect(P1_X, 0, PW, SH, COL_BG);
        GUI_FillRect(P2_X, 0, PW, SH, COL_BG);
        GUI_FillRect(bx, by, BS, BS, COL_BG);

        // Input
        char ch  = Keyboard_GetLastChar();
        uint8_t sc = Keyboard_GetLastScancode();

        if ((ch == 'w' || ch == 'W') && p1y > 40)             p1y -= 6;
        if ((ch == 's' || ch == 'S') && p1y < SH - PH - 4)    p1y += 6;
        if ((ch == 'o' || ch == 'O') && p2y > 40)             p2y -= 6;
        if ((ch == 'l' || ch == 'L') && p2y < SH - PH - 4)    p2y += 6;
        if (sc == 0x01) running = false;

        // Ball movement (every 2 frames)
        speed_cnt++;
        if (speed_cnt >= 2) {
            speed_cnt = 0;
            bx += bdx; by += bdy;

            // Wall bounce (top/bottom, excluding header)
            if (by <= 40)     { by = 40;       bdy = -bdy; }
            if (by >= SH - BS) { by = SH - BS; bdy = -bdy; }

            // Paddle 1 (left)
            if (bx <= P1_X + PW && bx >= P1_X &&
                by + BS >= p1y && by <= p1y + PH) {
                bdx = -bdx; bx = P1_X + PW + 1;
                // Add slight angle based on hit position
                int rel = (by + BS/2) - (p1y + PH/2);
                bdy = rel / 12;
                if (bdy == 0) bdy = (bdx > 0) ? 1 : -1;
            }

            // Paddle 2 (right)
            if (bx + BS >= P2_X && bx + BS <= P2_X + PW &&
                by + BS >= p2y && by <= p2y + PH) {
                bdx = -bdx; bx = P2_X - BS - 1;
                int rel = (by + BS/2) - (p2y + PH/2);
                bdy = rel / 12;
                if (bdy == 0) bdy = (bdx > 0) ? 1 : -1;
            }

            // Goals
            if (bx < 0) {
                s2++;
                bx = SW/2; by = SH/2;
                bdx = 3; bdy = 2;
                // Redraw center line after clear
                for(int y = 0; y < SH; y += 20)
                    GUI_FillRect(SW/2 - 2, y, 4, 10, COL_MID);
            }
            if (bx > SW) {
                s1++;
                bx = SW/2; by = SH/2;
                bdx = -3; bdy = -2;
                for(int y = 0; y < SH; y += 20)
                    GUI_FillRect(SW/2 - 2, y, 4, 10, COL_MID);
            }
        }

        // Draw
        GUI_FillRect(P1_X, p1y, PW, PH, COL_PADDLE);
        GUI_FillRect(P2_X, p2y, PW, PH, COL_PADDLE);
        GUI_FillRect(bx, by, BS, BS, COL_BALL);

        // Scores
        GUI_FillRect(200, 8, 200, 24, COL_BG);
        GUI_FillRect(640, 8, 200, 24, COL_BG);
        draw_num2(280, 16, s1);
        draw_num2(700, 16, s2);

        for(volatile int k = 0; k < 500000; k++);
    }
}
