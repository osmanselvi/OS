#pragma once

#include <stdint.h>

typedef struct {
    uint64_t Begin, Length;
    uint32_t Type;
    uint32_t ACPI;
} MemoryRegion;

typedef struct  {
    int RegionCount;
    MemoryRegion* Regions;
} MemoryInfo;

typedef struct {
    void* Address;
    uint32_t Width;
    uint32_t Height;
    uint32_t Pitch;
    uint8_t Bpp;
} FrameBufferInfo;

typedef struct {
    MemoryInfo Memory;
    uint8_t BootDevice;
    FrameBufferInfo FrameBuffer;
} BootParams;