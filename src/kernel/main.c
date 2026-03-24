#include <stdint.h>
#include "stdio.h"
#include "memory.h"
#include <hal/hal.h>
#include <arch/i686/irq.h>
#include <arch/i686/vesa.h>
#include <drivers/keyboard.h>
#include <drivers/mouse.h>
#include <gui/gui.h>
#include <fs/fs.h>
#include <apps/snake.h>
#include <apps/pong.h>
#include <apps/nadit.h>
#include <apps/shell.h>
#include <debug.h>
#include <boot/bootparams.h>
#include <stdbool.h>

extern void _init();

uint8_t __attribute__((aligned(16))) g_KernelStack[65536];

void kernel_main(BootParams* bootParams);

void __attribute__((naked)) start(BootParams* bootParams)
{
    __asm__ volatile (
        "movl 4(%%esp), %%eax\n"    // Load bootParams from old stack
        "movl %0, %%esp\n"          // Switch to new kernel stack
        "andl $-16, %%esp\n"        // Force 16-byte alignment
        "movl %%esp, %%ebp\n"       // Initialize frame pointer
        "pushl %%eax\n"             // Push bootParams to new stack
        "pushl $0\n"                // Dummy return address for call consistency
        "jmp kernel_main\n"         // Jump to actual kernel entry
        : : "g" (&g_KernelStack[65536]) : "eax", "memory"
    );
}

void kernel_main(BootParams* bootParams)
{   
    _init();
    HAL_Initialize();
    Memory_Initialize();
    fs_init();

    if (bootParams->FrameBuffer.Address == 0) {
        printf("FATAL: No framebuffer!\n");
        for(;;);
    }

    VESA_Initialize(&bootParams->FrameBuffer);
    GUI_Initialize(bootParams->FrameBuffer.Width, bootParams->FrameBuffer.Height);

    log_debug("Main", "GUI running at %dx%dx%d",
        bootParams->FrameBuffer.Width,
        bootParams->FrameBuffer.Height,
        bootParams->FrameBuffer.Bpp);

    // ── GUI Main Loop (restartable after apps) ───────────────────────────────
    while (1) {
        // ── Setup / Re-setup Desktop ─────────────────────────────────────────
        WM_Initialize();
        WM_CreateIcon("Not Defteri", "NADIT",  30, 60);
        WM_CreateIcon("Terminal",     "SHELL",  30, 140);
        WM_CreateIcon("Snake",        "SNAKE",  30, 220);
        WM_CreateIcon("Pong",         "PONG",   30, 300);
        WM_DrawAll();
        GUI_SwapBuffers(); // Initial draw commit

        // ── Event Loop ───────────────────────────────────────────────────────
        int gui_running = 1;
        int prev_mx = -1, prev_my = -1;
        bool prev_lbtn = false;
        uint32_t loop_count = 0;

        while (gui_running == WM_CONTINUE) {
            if (++loop_count % 1000000000 == 0) {
                // Reduced heartbeat frequency
            }
            MouseState ms;
            Mouse_GetState(&ms);
            bool lbtn = ms.leftButton;
            int mx = ms.x, my = ms.y;

            bool mouse_changed = (mx != prev_mx || my != prev_my || lbtn != prev_lbtn);
            if (mouse_changed) {
                gui_running = WM_HandleMouse(mx, my, lbtn);
                WM_DrawAll();
                GUI_SwapBuffers(); // Commit frame to VRAM
                prev_mx = mx; prev_my = my; prev_lbtn = lbtn;
            }

            char key = Keyboard_GetLastChar();
            if (key) {
                gui_running = WM_HandleKey(key);
                WM_DrawAll();
                GUI_SwapBuffers();
            }

            // Suspend CPU until next interrupt (Timer, Keyboard, Mouse)
            // This prevents busy-waiting and stabilizes emulated IRQ handling.
            __asm__ volatile ("hlt");
        }

        // ── Dispatch app ─────────────────────────────────────────────────────
        if (gui_running == WM_EXIT) {
            break;  // Kapat clicked
        } else if (gui_running == WM_SNAKE) {
            Snake_Run();
            WM_DrawAll(); GUI_SwapBuffers(); // Restore Desktop after app
        } else if (gui_running == WM_PONG) {
            Pong_Run();
            WM_DrawAll(); GUI_SwapBuffers();
        } else if (gui_running == WM_NADIT) {
            Nadit_Run("benioku.txt");
            WM_DrawAll(); GUI_SwapBuffers();
        }
    }

    // Shutdown screen
    VESA_ClearScreen(0x00000010);
    GUI_DrawString(380, 360, "SincanOs kapatiluyor...", 0x00AADDFF, 0xFFFFFFFF);
    GUI_DrawString(430, 384, "Gule gule!", COL_WHITE, 0xFFFFFFFF);

    for (;;);
}
