#pragma once
#include <stdint.h>
#include <boot/bootparams.h>

void VESA_Initialize(FrameBufferInfo* fbInfo);
void VESA_PutPixel(uint32_t x, uint32_t y, uint32_t color);
void VESA_ClearScreen(uint32_t color);
void VESA_FillRect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color);
void VESA_Blit(const uint32_t* buffer);
