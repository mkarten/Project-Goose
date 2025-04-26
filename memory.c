#include "memory.h"
#include "kernelStruct.h"

#define HEAP_START 0x100000    // Start heap at 1MB
#define MAGIC_NUMBER 0xDEADBEEF // For memory corruption detection

static MemoryManager memoryManager;

// Helper function to merge adjacent free blocks
static void merge_free_blocks(MemoryBlock* block) {
    while (block->next && block->next->state == MEMORY_BLOCK_FREE) {
        MemoryBlock* next = block->next;
        block->size += next->size;
        block->next = next->next;
        if (next->next) {
            next->next->prev = block;
        }
    }
}

void mem_init() {
    // Get entry count from 0x8A00
    uint16_t entries = *(uint16_t*)0x8A00;
    MemoryMapEntry* memMap = (MemoryMapEntry*)0x8A04;
    
    // Find largest usable memory region for heap
    uint32_t largestRegionSize = 0;
    uint32_t heapStart = HEAP_START;
    
    for (uint16_t i = 0; i < entries; i++) {
        if (memMap[i].type == MEMORY_TYPE_USABLE) {
            uint32_t regionStart = memMap[i].baseAddrLow;
            uint32_t regionSize = memMap[i].lengthLow;
            
            // Skip regions below HEAP_START
            if (regionStart < HEAP_START) {
                if (regionStart + regionSize > HEAP_START) {
                    regionSize = (regionStart + regionSize) - HEAP_START;
                    regionStart = HEAP_START;
                } else {
                    continue;
                }
            }
            
            if (regionSize > largestRegionSize) {
                largestRegionSize = regionSize;
                heapStart = regionStart;
            }
        }
    }
    
    // Initialize memory manager
    memoryManager.heapStart = heapStart;
    memoryManager.heapSize = largestRegionSize;
    memoryManager.usedMemory = sizeof(MemoryBlock);
    memoryManager.freeMemory = largestRegionSize - sizeof(MemoryBlock);
    
    // Create initial free block
    MemoryBlock* initialBlock = (MemoryBlock*)heapStart;
    initialBlock->magic = MAGIC_NUMBER;
    initialBlock->size = largestRegionSize;
    initialBlock->state = MEMORY_BLOCK_FREE;
    initialBlock->next = NULL;
    initialBlock->prev = NULL;
    
    memoryManager.firstBlock = initialBlock;
}

void* kmalloc(uint32_t size) {
    if (size == 0) return NULL;
    
    // Align size to 4 bytes
    size = (size + 3) & ~3;
    
    MemoryBlock* current = memoryManager.firstBlock;
    while (current) {
        if (current->state == MEMORY_BLOCK_FREE && current->size >= size + sizeof(MemoryBlock)) {
            // Split block if it's too large
            if (current->size > size + sizeof(MemoryBlock) + 32) { // 32 bytes minimum split
                MemoryBlock* newBlock = (MemoryBlock*)((uint32_t)current + sizeof(MemoryBlock) + size);
                newBlock->magic = MAGIC_NUMBER;
                newBlock->size = current->size - size - sizeof(MemoryBlock);
                newBlock->state = MEMORY_BLOCK_FREE;
                newBlock->next = current->next;
                newBlock->prev = current;
                
                if (current->next) {
                    current->next->prev = newBlock;
                }
                
                current->next = newBlock;
                current->size = size + sizeof(MemoryBlock);
            }
            
            current->state = MEMORY_BLOCK_USED;
            memoryManager.usedMemory += current->size;
            memoryManager.freeMemory -= current->size;
            
            return (void*)((uint32_t)current + sizeof(MemoryBlock));
        }
        current = current->next;
    }
    
    return NULL; // No suitable block found
}

void kfree(void* ptr) {
    if (!ptr) return;
    
    MemoryBlock* block = (MemoryBlock*)((uint32_t)ptr - sizeof(MemoryBlock));
    
    // Verify block integrity
    if (block->magic != MAGIC_NUMBER) {
        return; // Memory corruption detected
    }
    
    block->state = MEMORY_BLOCK_FREE;
    memoryManager.usedMemory -= block->size;
    memoryManager.freeMemory += block->size;
    
    // Merge with adjacent free blocks
    if (block->prev && block->prev->state == MEMORY_BLOCK_FREE) {
        block = block->prev;
    }
    merge_free_blocks(block);
}

void mem_stats(uint32_t* total, uint32_t* used, uint32_t* free) {
    if (total) *total = memoryManager.heapSize;
    if (used) *used = memoryManager.usedMemory;
    if (free) *free = memoryManager.freeMemory;
}

void print_memory_map() {
    struct Kernel* kernel = (struct Kernel*)0x00000000;
    uint16_t entries = *(uint16_t*)0x8A00;
    MemoryMapEntry* memMap = (MemoryMapEntry*)0x8A04;
    
    kernel->puts("Memory Map:\n");
    for (uint16_t i = 0; i < entries; i++) {
        kernel->puts("Base: 0x");
        kernel->puti(memMap[i].baseAddrLow);
        kernel->puts(" Length: 0x");
        kernel->puti(memMap[i].lengthLow);
        kernel->puts(" Type: ");
        kernel->puti(memMap[i].type);
        kernel->putc('\n');
    }
    
    kernel->puts("\nHeap Stats:\n");
    kernel->puts("Heap Start: 0x");
    kernel->puti(memoryManager.heapStart);
    kernel->puts("\nHeap Size: ");
    kernel->puti(memoryManager.heapSize);
    kernel->puts(" bytes\n");
    kernel->puts("Used Memory: ");
    kernel->puti(memoryManager.usedMemory);
    kernel->puts(" bytes\n");
    kernel->puts("Free Memory: ");
    kernel->puti(memoryManager.freeMemory);
    kernel->puts(" bytes\n");
}