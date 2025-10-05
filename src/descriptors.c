#include "../includes/descriptors.h"
#include "../includes/memory.h"
#include "io.h"

// GDT entries and pointer
static GDTEntry* gdt_entries;
static GDTPtr* gdt_ptr;

// IDT entries and pointer
static IDTEntry* idt_entries;
static IDTPtr* idt_ptr;

// External assembly functions
extern void gdt_flush(uint32_t);
extern void idt_load(uint32_t);
extern void keyboard_handler_wrapper();

#define PIC1_COMMAND 0x20
#define PIC1_DATA    0x21
#define PIC2_COMMAND 0xA0
#define PIC2_DATA    0xA1

#define ICW1_ICW4   0x01
#define ICW1_INIT   0x10
#define ICW4_8086   0x01

// Initialize PIC
static void pic_init() {
    // ICW1: start initialization sequence
    outportb(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4);
    outportb(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);

    // ICW2: remap IRQ vectors
    outportb(PIC1_DATA, 0x20);    // IRQ 0-7 -> interrupts 0x20-0x27
    outportb(PIC2_DATA, 0x28);    // IRQ 8-15 -> interrupts 0x28-0x2F

    // ICW3: tell PICs about each other
    outportb(PIC1_DATA, 0x04);    // Tell master PIC about slave at IRQ2
    outportb(PIC2_DATA, 0x02);    // Tell slave PIC its cascade identity

    // ICW4: set x86 mode
    outportb(PIC1_DATA, ICW4_8086);
    outportb(PIC2_DATA, ICW4_8086);

    // Clear IMR - enable all interrupts
    outportb(PIC1_DATA, 0x0);
    outportb(PIC2_DATA, 0x0);
}

static void gdt_set_gate(int32_t num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran) {
    gdt_entries[num].base_low = (base & 0xFFFF);
    gdt_entries[num].base_middle = (base >> 16) & 0xFF;
    gdt_entries[num].base_high = (base >> 24) & 0xFF;

    gdt_entries[num].limit_low = (limit & 0xFFFF);
    gdt_entries[num].granularity = ((limit >> 16) & 0x0F) | (gran & 0xF0);

    gdt_entries[num].access = access;
}

void gdt_init() {
    // Allocate memory for GDT entries and pointer
    gdt_entries = kmalloc(sizeof(GDTEntry) * GDT_NUM_ENTRIES);
    gdt_ptr = kmalloc(sizeof(GDTPtr));

    gdt_ptr->limit = (sizeof(GDTEntry) * GDT_NUM_ENTRIES) - 1;
    gdt_ptr->base = (uint32_t)gdt_entries;

    // Null segment
    gdt_set_gate(0, 0, 0, 0, 0);
    // Code segment
    gdt_set_gate(1, 0, 0xFFFFFFFF, 0x9A, 0xCF); // 0x9A: Present, Ring 0, Code segment
    // Data segment
    gdt_set_gate(2, 0, 0xFFFFFFFF, 0x92, 0xCF); // 0x92: Present, Ring 0, Data segment

    // Load the GDT
    gdt_flush((uint32_t)gdt_ptr);
}

static void idt_set_gate(uint8_t num, uint32_t base, uint16_t selector, uint8_t flags) {
    idt_entries[num].base_low = base & 0xFFFF;
    idt_entries[num].base_high = (base >> 16) & 0xFFFF;
    idt_entries[num].selector = selector;
    idt_entries[num].zero = 0;
    idt_entries[num].flags = flags;
}

static void default_handler() {
    // For now, just return
    __asm__ __volatile__("iret");
}

void idt_init() {

    // Disable interrupts while setting up
    __asm__ __volatile__("cli");

    // Allocate memory for IDT
    idt_entries = kmalloc(sizeof(IDTEntry) * 256);
    idt_ptr = kmalloc(sizeof(IDTPtr));

    idt_ptr->limit = (sizeof(IDTEntry) * 256) - 1;
    idt_ptr->base = (uint32_t)idt_entries;

    // Clear IDT and set default handler
    for (int i = 0; i < 256; i++) {
        idt_set_gate(i, (uint32_t)default_handler, 0x08, 0x8E);
    }

    // Set up keyboard interrupt (IRQ1 -> INT 0x21)
    idt_set_gate(0x21, (uint32_t)keyboard_handler_wrapper, 0x08, 0x8E);

    // Initialize PIC
    pic_init();


    // Enable keyboard interrupt
    outportb(PIC1_DATA, 0xFF);             // Disable all IRQs on master PIC
    outportb(PIC1_DATA, inportb(PIC1_DATA) & ~(1 << 1));
    outportb(PIC2_DATA, 0xFF);             // Disable all IRQs on slave PIC

    // Load IDT
    idt_load((uint32_t)idt_ptr);

    // Enable interrupts
    __asm__ __volatile__("sti");
}