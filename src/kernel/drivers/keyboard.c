#include "keyboard.h"
#include "keymaps.h"
#include <arch/i686/io.h>
#include <arch/i686/irq.h>
#include <stdio.h>
#include <stdbool.h>

static const char* g_LayoutLower = tr_q_map_lower;
static const char* g_LayoutUpper = tr_q_map_upper;

static volatile uint8_t g_LastScancode = 0;
static volatile char g_LastChar = 0;
static bool g_ShiftPressed = false;

static void Keyboard_Handler(Registers* regs)
{
    uint8_t scancode = i686_inb(0x60);

    // Shift Handling
    if (scancode == 0x2A || scancode == 0x36) {
        g_ShiftPressed = true;
        return;
    }
    if (scancode == 0xAA || scancode == 0xB6) {
        g_ShiftPressed = false;
        return;
    }

    // Key Release
    if (scancode & 0x80) {
        return;
    }

    // Key Press
    g_LastScancode = scancode;
    if (scancode < 128) {
        if (g_ShiftPressed) {
            g_LastChar = g_LayoutUpper[scancode];
        } else {
            g_LastChar = g_LayoutLower[scancode];
        }
    }
}

void Keyboard_Initialize()
{
    i686_IRQ_RegisterHandler(1, Keyboard_Handler);
    i686_IRQ_Unmask(1);
}

char Keyboard_GetLastChar()
{
    char c = g_LastChar;
    g_LastChar = 0;
    return c;
}

uint8_t Keyboard_GetLastScancode()
{
    uint8_t s = g_LastScancode;
    g_LastScancode = 0;
    return s;
}

void Keyboard_SetLayout(int layout)
{
    if (layout == 1) {
        g_LayoutLower = tr_q_map_lower;
        g_LayoutUpper = tr_q_map_upper;
    } else {
        g_LayoutLower = us_map_lower;
        g_LayoutUpper = us_map_upper;
    }
}
