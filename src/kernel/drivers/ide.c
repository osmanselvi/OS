#include "ide.h"
#include <hal/hal.h>
#include <arch/i686/io.h>
#include <stdio.h>
#include <debug.h>

#define IDE_DATA        0x1F0
#define IDE_ERROR       0x1F1
#define IDE_SECT_COUNT  0x1F2
#define IDE_LBA_LOW     0x1F3
#define IDE_LBA_MID     0x1F4
#define IDE_LBA_HIGH    0x1F5
#define IDE_DRIVE_SEL   0x1F6
#define IDE_COMMAND     0x1F7
#define IDE_STATUS      0x1F7

#define IDE_CMD_READ    0x20
#define IDE_CMD_WRITE   0x30
#define IDE_CMD_IDENTIFY 0xEC

static void IDE_WaitBusy() {
    while (i686_inb(IDE_STATUS) & 0x80);
}

static void IDE_WaitReady() {
    while (!(i686_inb(IDE_STATUS) & 0x40));
}

void IDE_Initialize() {
    log_debug("IDE", "Initializing Primary Master...");
}

int IDE_ReadSectors(uint32_t lba, uint8_t count, void* buffer) {
    log_debug("IDE", "Read LBA %d, count %d", lba, count);
    uint16_t* ptr = (uint16_t*)buffer;

    IDE_WaitBusy();
    i686_outb(IDE_DRIVE_SEL, 0xE0 | ((lba >> 24) & 0x0F));
    i686_outb(IDE_SECT_COUNT, count);
    i686_outb(IDE_LBA_LOW, (uint8_t)lba);
    i686_outb(IDE_LBA_MID, (uint8_t)(lba >> 8));
    i686_outb(IDE_LBA_HIGH, (uint8_t)(lba >> 16));
    i686_outb(IDE_COMMAND, IDE_CMD_READ);

    for (int i = 0; i < count; i++) {
        IDE_WaitBusy();
        IDE_WaitReady();
        for (int j = 0; j < 256; j++) {
            *ptr++ = i686_inw(IDE_DATA);
        }
    }
    return 0;
}

int IDE_WriteSectors(uint32_t lba, uint8_t count, const void* buffer) {
    log_debug("IDE", "Write LBA %d, count %d", lba, count);
    const uint16_t* ptr = (const uint16_t*)buffer;

    IDE_WaitBusy();
    i686_outb(IDE_DRIVE_SEL, 0xE0 | ((lba >> 24) & 0x0F));
    i686_outb(IDE_SECT_COUNT, count);
    i686_outb(IDE_LBA_LOW, (uint8_t)lba);
    i686_outb(IDE_LBA_MID, (uint8_t)(lba >> 8));
    i686_outb(IDE_LBA_HIGH, (uint8_t)(lba >> 16));
    i686_outb(IDE_COMMAND, IDE_CMD_WRITE);

    for (int i = 0; i < count; i++) {
        IDE_WaitBusy();
        IDE_WaitReady();
        for (int j = 0; j < 256; j++) {
            i686_outw(IDE_DATA, *ptr++);
        }
    }
    return 0;
}
