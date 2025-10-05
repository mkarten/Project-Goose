#ifndef DESCRIPTORS_H
#define DESCRIPTORS_H

#include "stdint.h"

// GDT entry structure
typedef struct {
    uint16_t limit_low;           // Lower 16 bits of limit
    uint16_t base_low;            // Lower 16 bits of base
    uint8_t  base_middle;         // Next 8 bits of base
    uint8_t  access;              // Access flags
    uint8_t  granularity;         // Granularity flags
    uint8_t  base_high;           // Last 8 bits of base
} __attribute__((packed)) GDTEntry;

// GDT pointer structure
typedef struct {
    uint16_t limit;               // GDT length - 1
    uint32_t base;                // GDT base address
} __attribute__((packed)) GDTPtr;

// IDT entry structure
typedef struct {
    uint16_t base_low;            // Lower 16 bits of handler address
    uint16_t selector;            // Kernel segment selector
    uint8_t  zero;               // Must be zero
    uint8_t  flags;              // Type and attributes
    uint16_t base_high;           // Upper 16 bits of handler address
} __attribute__((packed)) IDTEntry;

// IDT pointer structure
typedef struct {
    uint16_t limit;               // IDT length - 1
    uint32_t base;                // IDT base address
} __attribute__((packed)) IDTPtr;

// Initialize GDT
void gdt_init();

// Initialize IDT (to be implemented later)
void idt_init();

// GDT segment numbers
#define GDT_NULL_SEG    0
#define GDT_CODE_SEG    1
#define GDT_DATA_SEG    2
#define GDT_NUM_ENTRIES 3

#endif // DESCRIPTORS_H