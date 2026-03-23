#pragma once
#include <stdint.h>

void IDE_Initialize();
int  IDE_ReadSectors(uint32_t lba, uint8_t count, void* buffer);
int  IDE_WriteSectors(uint32_t lba, uint8_t count, const void* buffer);
