#include "memory.h"
#include <stdio.h>

extern uint8_t __end;

typedef struct mem_block {
    size_t size;
    int is_free;
    struct mem_block *next;
} mem_block_t;

static mem_block_t* g_HeapHead = NULL;
static void* g_HeapStart = NULL;

void Memory_Initialize()
{
    // Align heap start to 4KB boundary
    g_HeapStart = (void*)(((uint32_t)&__end + 0xFFF) & ~0xFFF);
    
    // We'll give it a 4MB initial heap
    size_t initial_heap_size = 4 * 1024 * 1024;
    
    g_HeapHead = (mem_block_t*)g_HeapStart;
    g_HeapHead->size = initial_heap_size - sizeof(mem_block_t);
    g_HeapHead->is_free = 1;
    g_HeapHead->next = NULL;
    
    printf("Memory: Heap initialized at %p (size %d MB)\n", g_HeapStart, initial_heap_size / (1024 * 1024));
}

void* kmalloc(size_t size)
{
    mem_block_t *curr = g_HeapHead;
    while (curr) {
        if (curr->is_free && curr->size >= size) {
            /* Split block if enough space */
            if (curr->size > size + sizeof(mem_block_t) + 8) { // 8 byte min leftover
                mem_block_t *new_block = (mem_block_t *)((uint8_t *)curr + sizeof(mem_block_t) + size);
                new_block->size = curr->size - size - sizeof(mem_block_t);
                new_block->is_free = 1;
                new_block->next = curr->next;
                
                curr->size = size;
                curr->next = new_block;
            }
            curr->is_free = 0;
            return (void *)((uint8_t *)curr + sizeof(mem_block_t));
        }
        curr = curr->next;
    }
    
    printf("Memory: KERNEL PANIC - OUT OF MEMORY trying to allocate %d bytes!\n", size);
    return NULL;
}

void kfree(void *ptr)
{
    if (!ptr) return;
    mem_block_t *block = (mem_block_t *)((uint8_t *)ptr - sizeof(mem_block_t));
    block->is_free = 1;
    
    /* Simple coalescing with NEXT block if free */
    if (block->next && block->next->is_free) {
        block->size += block->next->size + sizeof(mem_block_t);
        block->next = block->next->next;
    }
    
    // Note: To be fully robust, we'd also need to coalesce with the PREVIOUS block.
    // However, this simple version is what SincanOs used and is stable for now.
}

void* memcpy(void* dst, const void* src, size_t num)
{
    uint32_t* d32 = (uint32_t*)dst;
    const uint32_t* s32 = (const uint32_t*)src;
    size_t n32 = num / 4;
    while (n32--) *d32++ = *s32++;

    uint8_t* d8 = (uint8_t*)d32;
    const uint8_t* s8 = (const uint8_t*)s32;
    size_t n8 = num % 4;
    while (n8--) *d8++ = *s8++;

    return dst;
}

void * memset(void * ptr, int value, size_t num)
{
    uint8_t* u8Ptr = (uint8_t *)ptr;
    for (size_t i = 0; i < num; i++) u8Ptr[i] = (uint8_t)value;
    return ptr;
}

int memcmp(const void* ptr1, const void* ptr2, size_t num)
{
    const uint8_t* u8Ptr1 = (const uint8_t *)ptr1;
    const uint8_t* u8Ptr2 = (const uint8_t *)ptr2;
    for (size_t i = 0; i < num; i++)
        if (u8Ptr1[i] != u8Ptr2[i]) return 1;
    return 0;
}
