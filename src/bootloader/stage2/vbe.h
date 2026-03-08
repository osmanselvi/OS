#pragma once
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    char VbeSignature[4];             // == "VESA"
    uint16_t VbeVersion;                 // == 0x0300 for VBE 3.0
    uint16_t OemStringPtr[2];            // isa vbeFarPtr
    uint8_t Capabilities[4];
    uint16_t VideoModePtr[2];            // isa vbeFarPtr
    uint16_t TotalMemory;                // as # of 64KB blocks
    uint16_t OemSoftwareRev;
    uint32_t OemVendorNamePtr;
    uint32_t OemProductNamePtr;
    uint32_t OemProductRevPtr;
    uint8_t Reserved[222];
    uint8_t OemData[256];
} __attribute__((packed)) VbeInfoBlock;

typedef struct {
    uint16_t Attributes;
    uint8_t WindowA, WindowB;
    uint16_t Granularity;
    uint16_t WindowSize;
    uint16_t SegmentA, SegmentB;
    uint32_t WinFuncPtr;
    uint16_t Pitch;
    uint16_t Width;
    uint16_t Height;
    uint8_t WChar, HChar, Planes, BitsPerPixel, Banks;
    uint8_t MemoryModel, BankSize, ImagePages;
    uint8_t Reserved0;
    
    uint8_t RedMaskSize, RedFieldPosition;
    uint8_t GreenMaskSize, GreenFieldPosition;
    uint8_t BlueMaskSize, BlueFieldPosition;
    uint8_t RsvdMaskSize, RsvdFieldPosition;
    uint8_t DirectColorModeInfo;
    
    uint32_t PhysicalAddress;           // physical address of flat memory frame buffer
    uint32_t Reserved1;
    uint16_t Reserved2;
} __attribute__((packed)) VbeModeInfoBlock;

#include <boot/bootparams.h>
bool VBE_Initialize(FrameBufferInfo* fbInfo, uint16_t targetWidth, uint16_t targetHeight, uint8_t targetBpp);
