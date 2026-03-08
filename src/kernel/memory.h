#pragma once
#include <stdint.h>
#include <stddef.h>

void Memory_Initialize();
void* kmalloc(size_t size);
void kfree(void* ptr);

void* memcpy(void* dst, const void* src, size_t num);
void* memset(void* ptr, int value, size_t num);
int   memcmp(const void* ptr1, const void* ptr2, size_t num);
