#include <stdint.h>
#include "stdio.h"
#include "x86.h"
#include "disk.h"
#include "fat.h"
#include "memdefs.h"
#include "memory.h"
#include "mbr.h"
#include "stdlib.h"
#include "string.h"
#include "elf.h"
#include "memdetect.h"
#include "vbe.h"
#include <boot/bootparams.h>

uint8_t* KernelLoadBuffer = (uint8_t*)MEMORY_LOAD_KERNEL;
uint8_t* Kernel = (uint8_t*)MEMORY_KERNEL_ADDR;

BootParams g_BootParams;

typedef void (*KernelStart)(BootParams* bootParams);

void __attribute__((cdecl)) start(uint16_t bootDrive, void* partition)
{
    clrscr();

    DISK disk;
    if (!DISK_Initialize(&disk, bootDrive))
    {
        printf("Disk init error\r\n");
        goto end;
    }

    Partition part;
    MBR_DetectPartition(&part, &disk, partition);

    if (!FAT_Initialize(&part))
    {
        printf("FAT init error\r\n");
        goto end;
    }

    // prepare boot params
    g_BootParams.BootDevice = bootDrive;
    Memory_Detect(&g_BootParams.Memory);

    // Initialize VBE
    if (!VBE_Initialize(&g_BootParams.FrameBuffer, 1024, 768, 32)) {
        if (!VBE_Initialize(&g_BootParams.FrameBuffer, 800, 600, 32)) {
             printf("VBE: Failed to initialize graphics!\r\n");
        }
    }

    // load kernel
    KernelStart kernelEntry;
    if (!ELF_Read(&part, "/boot/kernel.elf", (void**)&kernelEntry))
    {
        printf("ELF read failed, booting halted!\r\n");
        goto end;
    }

    // execute kernel
    kernelEntry(&g_BootParams);

end:
    for (;;);
}
