#include "vesa.h"

static FrameBufferInfo g_FB;

void VESA_Initialize(FrameBufferInfo* fbInfo)
{
    g_FB = *fbInfo;
}

void VESA_PutPixel(uint32_t x, uint32_t y, uint32_t color)
{
    if (x >= g_FB.Width || y >= g_FB.Height) return;

    if (g_FB.Bpp == 32)
    {
        uint32_t* pixelPtr = (uint32_t*)((uint8_t*)g_FB.Address + y * g_FB.Pitch + x * 4);
        *pixelPtr = color;
    }
    else if (g_FB.Bpp == 24)
    {
        uint8_t* pixelPtr = (uint8_t*)g_FB.Address + y * g_FB.Pitch + x * 3;
        pixelPtr[0] = color & 0xFF;
        pixelPtr[1] = (color >> 8) & 0xFF;
        pixelPtr[2] = (color >> 16) & 0xFF;
    }
}

void VESA_ClearScreen(uint32_t color)
{
    VESA_FillRect(0, 0, g_FB.Width, g_FB.Height, color);
}

void VESA_FillRect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color)
{
    if (x >= g_FB.Width || y >= g_FB.Height) return;
    if (x + w > g_FB.Width) w = g_FB.Width - x;
    if (y + h > g_FB.Height) h = g_FB.Height - y;

    if (g_FB.Bpp == 32)
    {
        for (uint32_t i = 0; i < h; i++)
        {
            uint32_t* pixelPtr = (uint32_t*)((uint8_t*)g_FB.Address + (y + i) * g_FB.Pitch + x * 4);
            for (uint32_t j = 0; j < w; j++)
            {
                pixelPtr[j] = color;
            }
        }
    }
    else if (g_FB.Bpp == 24)
    {
        for (uint32_t i = 0; i < h; i++)
        {
            uint8_t* rowPtr = (uint8_t*)g_FB.Address + (y + i) * g_FB.Pitch + x * 3;
            for (uint32_t j = 0; j < w; j++)
            {
                uint8_t* p = rowPtr + j * 3;
                p[0] = color & 0xFF;
                p[1] = (color >> 8) & 0xFF;
                p[2] = (color >> 16) & 0xFF;
            }
        }
    }
}

#include <memory.h>

void VESA_Blit(const uint32_t* buffer)
{
    if (g_FB.Bpp == 32)
    {
        for (uint32_t y = 0; y < g_FB.Height; y++)
        {
            void* fbPtr = (uint8_t*)g_FB.Address + y * g_FB.Pitch;
            const void* srcPtr = &buffer[y * g_FB.Width];
            memcpy(fbPtr, srcPtr, g_FB.Width * 4);
        }
    }
    else if (g_FB.Bpp == 24)
    {
        // ... bits for 24-bit if needed (usually custom packing required)
    }
}
