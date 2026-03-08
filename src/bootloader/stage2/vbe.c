#include "vbe.h"
#include "x86.h"
#include "stdio.h"
#include <boot/bootparams.h>
#include "memory.h"

// Use fixed buffers in conventional memory for BIOS calls
// 0x8000 and 0x9000 are safe (below 640K, after stage2 code)
#define VBE_INFO_BUF ((VbeInfoBlock*)0x8000)
#define VBE_MODE_BUF ((VbeModeInfoBlock*)0x9000)

bool VBE_GetModeInfo(uint16_t mode, VbeModeInfoBlock* modeInfo)
{
    if (x86_Video_GetModeInfo(mode, VBE_MODE_BUF)) {
        *modeInfo = *VBE_MODE_BUF;
        return true;
    }
    return false;
}

bool VBE_SetMode(uint16_t mode)
{
    return x86_Video_SetMode(mode);
}

bool VBE_Initialize(FrameBufferInfo* fbInfo, uint16_t targetWidth, uint16_t targetHeight, uint8_t targetBpp)
{
    VbeInfoBlock* vbeInfo = VBE_INFO_BUF;
    
    // Some BIOSes need "VBE2" initially to provide VBE 2.0+ info
    memcpy(vbeInfo->VbeSignature, "VBE2", 4);

    if (!x86_Video_GetVbeInfo(vbeInfo)) {
        printf("VBE: Failed to get VBE info\r\n");
        return false;
    }

    if (memcmp(vbeInfo->VbeSignature, "VESA", 4) != 0) {
        printf("VBE: Not VESA compatible (%.4s)!\r\n", vbeInfo->VbeSignature);
        return false;
    }

    // Get modes list
    uint32_t modes_addr = (vbeInfo->VideoModePtr[1] << 4) + vbeInfo->VideoModePtr[0];
    uint16_t* modes = (uint16_t*)modes_addr;

    uint16_t selectedMode = 0xFFFF;
    VbeModeInfoBlock selectedModeInfo;

    // We prioritize 1024x768x32, then 800x600x32, then any high-res mode
    uint16_t fallbackMode = 0xFFFF;
    VbeModeInfoBlock fallbackModeInfo;

    for (int i = 0; modes[i] != 0xFFFF; i++) {
        VbeModeInfoBlock modeInfo;
        if (VBE_GetModeInfo(modes[i], &modeInfo)) {
            // Must be linear framebuffer (bit 7)
            if (!(modeInfo.Attributes & (1 << 7)))
                continue;
            
            // Exact match?
            if (modeInfo.Width == targetWidth && modeInfo.Height == targetHeight && modeInfo.BitsPerPixel == targetBpp)
            {
                selectedMode = modes[i];
                selectedModeInfo = modeInfo;
                break;
            }

            // High-res fallback (highest resolution found so far)
            if (modeInfo.Width >= 800 && modeInfo.Height >= 600)
            {
                if (fallbackMode == 0xFFFF || (modeInfo.Width > fallbackModeInfo.Width) || 
                   (modeInfo.Width == fallbackModeInfo.Width && modeInfo.BitsPerPixel > fallbackModeInfo.BitsPerPixel))
                {
                    fallbackMode = modes[i];
                    fallbackModeInfo = modeInfo;
                }
            }
        }
    }

    if (selectedMode == 0xFFFF && fallbackMode != 0xFFFF)
    {
        selectedMode = fallbackMode;
        selectedModeInfo = fallbackModeInfo;
    }

    if (selectedMode == 0xFFFF) {
        printf("VBE: No suitable high-res mode found.\r\n");
        return false;
    }

    // Set the mode (0x4000 bit enables Linear Frame Buffer)
    if (!VBE_SetMode(selectedMode | 0x4000)) {
        printf("VBE: Failed to set mode 0x%x\r\n", selectedMode);
        return false;
    }

    // Populate FrameBufferInfo
    fbInfo->Address = (void*)selectedModeInfo.PhysicalAddress;
    fbInfo->Width = selectedModeInfo.Width;
    fbInfo->Height = selectedModeInfo.Height;
    fbInfo->Pitch = selectedModeInfo.Pitch;
    fbInfo->Bpp = selectedModeInfo.BitsPerPixel;

    printf("VBE: Started %dx%dx%d at %p\r\n", fbInfo->Width, fbInfo->Height, fbInfo->Bpp, fbInfo->Address);

    return true;
}
