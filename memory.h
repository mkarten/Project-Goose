#ifndef MEMORY_H
#define MEMORY_H

#include "stdint.h"
#include "stdbool.h"

// Memory block state
#define MEMORY_BLOCK_FREE     1
#define MEMORY_BLOCK_USED     2

// Memory region types from E820
#define MEMORY_TYPE_USABLE            1
#define MEMORY_TYPE_RESERVED          2
#define MEMORY_TYPE_ACPI_RECLAIMABLE  3
#define MEMORY_TYPE_ACPI_NVS         4
#define MEMORY_TYPE_BAD              5

// Memory block header structure
typedef struct MemoryBlock {
    uint32_t magic;           // Magic number to detect corruption
    uint32_t size;           // Size of the block including header
    uint8_t state;           // MEMORY_BLOCK_FREE or MEMORY_BLOCK_USED
    struct MemoryBlock* next; // Next block in the list
    struct MemoryBlock* prev; // Previous block in the list
} __attribute__((packed)) MemoryBlock;

// Memory region structure (from E820)
typedef struct {
    uint32_t baseAddrLow;
    uint32_t baseAddrHigh;
    uint32_t lengthLow;
    uint32_t lengthHigh;
    uint32_t type;
    uint32_t acpiExt;
} __attribute__((packed)) MemoryMapEntry;

// Memory manager structure
typedef struct {
    MemoryBlock* firstBlock;    // First block in the heap
    uint32_t heapStart;         // Start address of heap
    uint32_t heapSize;          // Total size of heap
    uint32_t usedMemory;        // Amount of used memory
    uint32_t freeMemory;        // Amount of free memory
} MemoryManager;

// Initialize memory manager
void mem_init();

// Allocate memory
void* kmalloc(uint32_t size);

// Free memory
void kfree(void* ptr);

// Get memory stats
void mem_stats(uint32_t* total, uint32_t* used, uint32_t* free);

// Print memory map information
void print_memory_map();

#endif // MEMORY_H