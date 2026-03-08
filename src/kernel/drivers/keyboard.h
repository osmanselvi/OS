#pragma once
#include <stdint.h>

void Keyboard_Initialize();
char Keyboard_GetLastChar();
uint8_t Keyboard_GetLastScancode();
void Keyboard_SetLayout(int layout); // 0=US, 1=TR-Q
