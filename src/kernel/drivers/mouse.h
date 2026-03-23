#pragma once
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    int x;
    int y;
    bool leftButton;
    bool rightButton;
    bool middleButton;
} MouseState;

void Mouse_Initialize();
void Mouse_GetState(MouseState* state);
uint32_t   Mouse_GetInterruptCount();
