#include "mouse.h"
#include <arch/i686/io.h>
#include <arch/i686/irq.h>
#include <stdio.h>
#include <stdbool.h>
#include <debug.h>

static volatile MouseState g_MouseState = {512, 384, false, false, false};
static uint8_t g_MouseCycle = 0;
static int8_t g_MousePacket[3];
static volatile uint32_t g_MouseInterruptCount = 0;

static void Mouse_Wait(uint8_t type)
{
    uint32_t timeout = 100000;
    if (type == 0) {
        while (timeout--) { if ((i686_inb(0x64) & 1) == 1) return; }
    } else {
        while (timeout--) { if ((i686_inb(0x64) & 2) == 0) return; }
    }
}

static void Mouse_Write(uint8_t data)
{
    Mouse_Wait(1);
    i686_outb(0x64, 0xD4);
    Mouse_Wait(1);
    i686_outb(0x60, data);
}

static uint8_t Mouse_Read()
{
    Mouse_Wait(0);
    return i686_inb(0x60);
}

static void Mouse_Handler(Registers* regs)
{
    g_MouseInterruptCount++;
    
    uint8_t status = i686_inb(0x64);
    if (!(status & 0x01)) return; // No data

    uint8_t data = i686_inb(0x60);
    if (!(status & 0x20)) return; // Not mouse data

    // Sync check: bit 3 of first byte should be 1
    if (g_MouseCycle == 0 && !(data & 0x08)) {
        // log_warn("Mouse", "Sync lost! Skip 0x%x", (uint32_t)data);
        return;
    }

    g_MousePacket[g_MouseCycle++] = data;

    if (g_MouseCycle == 3) {
        g_MouseCycle = 0;
        uint8_t flags = g_MousePacket[0];
        int8_t x_rel = g_MousePacket[1];
        int8_t y_rel = g_MousePacket[2];
        
        g_MouseState.x += x_rel;
        g_MouseState.y -= y_rel;
        
        if (g_MouseState.x < 0) g_MouseState.x = 0;
        if (g_MouseState.x > 1023) g_MouseState.x = 1023;
        if (g_MouseState.y < 0) g_MouseState.y = 0;
        if (g_MouseState.y > 767) g_MouseState.y = 767;

        g_MouseState.leftButton = (flags & 0x01);
        g_MouseState.rightButton = (flags & 0x02);
    }
}

void Mouse_Drain()
{
    while (i686_inb(0x64) & 0x01) {
        i686_inb(0x60);
        i686_iowait();
    }
}

void Mouse_Initialize()
{
    i686_IRQ_RegisterHandler(12, Mouse_Handler);
    i686_IRQ_Unmask(12);

    Mouse_Drain(); // Clear any junk

    // Enable mouse
    Mouse_Wait(1);
    i686_outb(0x64, 0xA8);

    // Enable interrupts
    Mouse_Wait(1);
    i686_outb(0x64, 0x20);
    Mouse_Wait(0);
    uint8_t status = i686_inb(0x60) | 0x02;
    Mouse_Wait(1);
    i686_outb(0x64, 0x60);
    Mouse_Wait(1);
    i686_outb(0x60, status);

    // Reset/Defaults
    Mouse_Write(0xF6); Mouse_Read(); 
    Mouse_Write(0xF4); Mouse_Read();

    Mouse_Drain(); // Final drain
    g_MouseCycle = 0;
}

MouseState Mouse_GetState()
{
    MouseState state;
    state.x = g_MouseState.x;
    state.y = g_MouseState.y;
    state.leftButton = g_MouseState.leftButton;
    state.rightButton = g_MouseState.rightButton;
    state.middleButton = g_MouseState.middleButton;
    return state;
}

uint32_t Mouse_GetInterruptCount()
{
    return g_MouseInterruptCount;
}
