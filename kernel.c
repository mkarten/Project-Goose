#include "stdint.h"
#include "stdbool.h"
#include "kernelStruct.h"
#include "colors.h"
#include "memory.h"
#include "keyboard.h"  // Added missing include
#include "descriptors.h"

void __cdecl c_entry_() {
    __asm__ __volatile__("mov $0x10, %ax\n\t"
                         "mov %ax, %ds\n\t"
                         "mov %ax, %es\n\t"
                         "mov %ax, %fs\n\t"
                         "mov %ax, %gs\n\t"
                         "mov %ax, %ss\n\t"
                         "mov $0x90000, %ebp\n\t"
                         "mov $0x50000, %esp\n\t");
    k_init();
    struct Kernel *kernel = (struct Kernel *)0x00000000;
    kernel->clear(Black);
    k_entrypoint();
}

int inportb(unsigned int port)
{
    int ret;
    __asm__ __volatile__("inb %%dx,%%al":"=a"(ret):"d"(port));
    return ret;
}

void outportb(unsigned int port,unsigned char value)
{
    __asm__ __volatile__("outb %%al,%%dx"::"a"(value),"d"(port));
}

void enable_bliking_text()
{
    inportb(0x3DA);
    outportb(0x3C0,0x30);
    outportb(0x3C0, inportb(0x3C1) | 0x08);
}

void disable_bliking_text()
{
    inportb(0x3DA);
    outportb(0x3C0,0x30);
    outportb(0x3C0, inportb(0x3C1) & 0xF7);
}


void enable_cursor(uint8_t cursor_start, uint8_t cursor_end)
{
    outportb(0x3D4, 0x0A);
    outportb(0x3D5, (inportb(0x3D5) & 0xC0) | cursor_start);

    outportb(0x3D4, 0x0B);
    outportb(0x3D5, (inportb(0x3D5) & 0xE0) | cursor_end);
}

void disable_cursor()
{
    outportb(0x3D4, 0x0A);
    outportb(0x3D5, 0x20);
}

void move_cursor(uint16_t x, uint16_t y)
{
    uint16_t pos = y * 80 + x;
    outportb(0x3D4, 0x0F);
    outportb(0x3D5, (uint8_t) (pos & 0xFF));
    outportb(0x3D4, 0x0E);
    outportb(0x3D5, (uint8_t) ((pos >> 8) & 0xFF));
}
// sleep for time milliseconds
void sleep(uint32_t time) {
    for (uint32_t i = 0; i < time; i++) {
        __asm__ __volatile__("nop");
    }
}

void put(unsigned char c,uint8_t forecolor,uint8_t backcolor,uint16_t x,uint16_t y) {
    struct Kernel *kernel = (struct Kernel *)0x00000000;
    uint16_t *video_memory = (uint16_t *)0xb8000;
    video_memory[x + y * kernel->ScreenWidth] = (backcolor << 12) | (forecolor << 8) | c;
}

void putc(unsigned char c) {
    struct Kernel *kernel = (struct Kernel *)0x00000000;
    // check if c is a control character
    if (c == '\n') {
        kernel->ScreenPosX = 0;
        kernel->ScreenPosY++;
    }else if (c == '\t') {
        kernel->ScreenPosX += 4;
    }else if (c == '\b') {
        if (kernel->ScreenPosX > 0) {
            kernel->ScreenPosX--;
        }else{
            kernel->ScreenPosY--;
            kernel->ScreenPosX = kernel->ScreenWidth - 1;
        }
        put(' ', kernel->ForegroundColor, kernel->BackgroundColor, kernel->ScreenPosX, kernel->ScreenPosY);
    } else {
        put(c, kernel->ForegroundColor, kernel->BackgroundColor, kernel->ScreenPosX, kernel->ScreenPosY);
        kernel->ScreenPosX++;
    }
    if (kernel->ScreenPosX >= kernel->ScreenWidth) {
        kernel->ScreenPosX = 0;
        kernel->ScreenPosY++;
    }

    // Handle scrolling if we've reached the bottom of the screen
    if (kernel->ScreenPosY >= kernel->ScreenHeight) {
        // Scroll the screen up by one line
        for (uint16_t y = 0; y < kernel->ScreenHeight - 1; y++) {
            for (uint16_t x = 0; x < kernel->ScreenWidth; x++) {
                // Copy each character from the line below to the current line
                uint16_t *video_memory = (uint16_t *)0xb8000;
                video_memory[y * kernel->ScreenWidth + x] = video_memory[(y + 1) * kernel->ScreenWidth + x];
            }
        }

        // Clear the last line
        for (uint16_t x = 0; x < kernel->ScreenWidth; x++) {
            put(' ', kernel->ForegroundColor, kernel->BackgroundColor, x, kernel->ScreenHeight - 1);
        }

        // Reset cursor position to the beginning of the last line
        kernel->ScreenPosY = kernel->ScreenHeight - 1;
    }

    move_cursor(kernel->ScreenPosX, kernel->ScreenPosY);
}

void clear(uint8_t color) {
    struct Kernel *kernel = (struct Kernel *)0x00000000;
    uint8_t back_color = kernel->BackgroundColor;
    kernel->BackgroundColor = color;
    for (uint16_t i = 0; i < 80 * 25; i++) {
        kernel->putc(' ');
    }
    kernel->BackgroundColor = back_color;
    kernel->ScreenPosX = 0;
    kernel->ScreenPosY = 0;
}


void puts(unsigned char str[]) {
    struct Kernel *kernel = (struct Kernel *)0x00000000;
    for (uint16_t i = 0; str[i] != '\0'; i++) {
        kernel->putc(str[i]);
    }
}

void puti(int32_t num) {
    struct Kernel *kernel = (struct Kernel *)0x00000000;
    if (num < 0) {
        kernel->putc('-');
        num = -num;
    }
    if (num == 0) {
        kernel->putc('0');
        return;
    }
    unsigned char str[10];
    uint8_t i = 0;
    while (num > 0) {
        str[i] = (num % 10) + '0';
        num /= 10;
        i++;
    }
    for (uint8_t j = i; j > 0; j--) {
        kernel->putc(str[j - 1]);
    }
}

void k_init() {
    // store static kernel struct in memory at 0x00000000
    struct Kernel *kernel = (struct Kernel *)0x00000000;

    // initialize kernel variables
    kernel->ScreenWidth = 80;
    kernel->ScreenHeight = 25;
    kernel->ScreenPosX = 0;
    kernel->ScreenPosY = 0;
    kernel->ForegroundColor = 0x07;
    kernel->BackgroundColor = 0x00;

    // initialize memory manager
    mem_init();

    // Initialize GDT
    gdt_init();

    // Initialize IDT structures (but don't set up interrupts yet)
    idt_init();

    // initialize keyboard system
    keyboard_init();

    // initialize kernel functions
    kernel->putc = putc+0xA000;
    kernel->clear = clear+0xA000;
    kernel->sleep = sleep+0xA000;
    kernel->puts = puts+0xA000;
    kernel->puti = puti+0xA000;
}

unsigned int restart_keyboard()
{
   int data = inportb(0x61);
   outportb(0x61,data | 0x80);//Disables the keyboard
   outportb(0x61,data & 0x7F);//Enables the keyboard
   return 0;
}

unsigned char get_scancode()
{
    unsigned char inputdata;
    // wait for input buffer to be full
    while((inportb(0x64) & 0x01) != 0x01);
    // read input buffer
    inputdata = inportb(0x60);
    // return input data
    return inputdata;
}

void k_entrypoint() {
    unsigned char Klayout[128] =
    {
        0,0,'1','2','3','4','5','6','7','8','9','0','-','=','\b','\t','q','w','e','r','t','y','u','i','o','p','[',']','\n',        0,'a','s','d','f','g','h','j','k','l',';','\'','`',0,'\\','z','x','c','v','b','n','m',',','.','/',0,'*',0,' ',
    };
    // the intro screen is 18 lines high and 12 columns wide
    unsigned char introScreen[19][12] = {
            {0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,},
            {0xFF, 0xFF, 0x00, 0x0F, 0x0F, 0x0F, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,},
            {0xFF, 0x00, 0x66, 0x0F, 0x00, 0x0F, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,},
            {0x00, 0x66, 0x66, 0x0F, 0x00, 0x0F, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,},
            {0xFF, 0x00, 0x00, 0x0F, 0x0F, 0x0F, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,},
            {0xFF, 0xFF, 0xFF, 0x00, 0x0F, 0x0F, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,},
            {0xFF, 0xFF, 0xFF, 0x00, 0x0F, 0x0F, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,},
            {0xFF, 0xFF, 0xFF, 0x00, 0x0F, 0x0F, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,},
            {0xFF, 0xFF, 0x00, 0x0F, 0x0F, 0x0F, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,},
            {0xFF, 0x00, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x00, 0xFF, 0xFF, 0xFF, 0x00,},
            {0xFF, 0x00, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x00, 0x00, 0x00, 0x00,},
            {0xFF, 0x00, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x00,},
            {0xFF, 0x00, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x00, 0xFF,},
            {0xFF, 0xFF, 0x00, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x00, 0xFF, 0xFF,},
            {0xFF, 0xFF, 0xFF, 0x00, 0x0F, 0x0F, 0x0F, 0x0F, 0x00, 0xFF, 0xFF, 0xFF,},
            {0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF,},
            {0xFF, 0xFF, 0xFF, 0xFF, 0x66, 0xFF, 0xFF, 0x66, 0xFF, 0xFF, 0xFF, 0xFF,},
            {0xFF, 0xFF, 0xFF, 0x66, 0x66, 0xFF, 0x66, 0x66, 0xFF, 0xFF, 0xFF, 0xFF}
    };
    struct Kernel *kernel = (struct Kernel *)0x00000000;
    kernel->clear(Black);

    unsigned char start[] = "Press any key to start kernel execution...";
    // center text on screen
    kernel->ScreenPosY = 12;
    kernel->ScreenPosX = (kernel->ScreenWidth - sizeof(start)) / 2;
    kernel->puts(start);
    unsigned char temp = get_scancode();
    unsigned char back_color = 0x07;
    kernel->clear(back_color);
    kernel->BackgroundColor = back_color;
    kernel->ForegroundColor = Black;
    disable_cursor();

    // display intro screen
    unsigned char Project[] = "PROJECT GOOSE";
    // disable blinking to have the full 16 background colors
    disable_bliking_text();
    // center the intro screen

    int xOff = (kernel->ScreenWidth - 24) / 2;
    int yOff = (kernel->ScreenHeight - 18) / 2;
    //__asm__ __volatile__("hlt");
    for (uint8_t y = 0; y < 18; y++) {

        for (uint8_t x = 0; x < 24; x++) {

            if (introScreen[y][x / 2] == 0xFF) {
                put(' ', kernel->ForegroundColor, kernel->BackgroundColor, x + xOff, y + yOff);
            }else{
                if (x % 2 == 0) {
                put(' ', introScreen[y][x / 2] | 0x00, introScreen[y][x / 2] | 0x00, x + xOff, y + yOff);
                } else {
                    put(' ', introScreen[y][x / 2] | 0x00, introScreen[y][x / 2] | 0x00, x + xOff, y + yOff);
                }
            }
        }
    }
    // display project goose centered under intro screen
    kernel->ScreenPosY = yOff + 19;
    kernel->ScreenPosX = (kernel->ScreenWidth - sizeof(Project)) / 2;
    kernel->puts(Project);
    // enable blinking text again
    // enable_bliking_text();
    // enable_cursor(15, 15);
    // move_cursor(79, 24);

    // restart the keyboard
    restart_keyboard();
    // move the cursor to the top left corner
    move_cursor(0, 0);
    // enable the cursor
    enable_bliking_text();    // go back to the top left corner
    kernel->ScreenPosX = 0;
    kernel->ScreenPosY = 0;
    // print the available memory
    unsigned char memMapTitle[] = "Memory Map (INT 0x15, EAX=0xE820):\n";
    unsigned char entryCount[] = "Entries found: ";
    unsigned char baseAddrStr[] = "Base Address: 0x";
    unsigned char lengthStr[] = "Length: 0x";
    unsigned char typeStr[] = "Type: ";
    unsigned char typeUnknown[] = "Unknown";
    unsigned char typeUsable[] = "Usable RAM";
    unsigned char typeReserved[] = "Reserved";
    unsigned char typeACPIReclaimable[] = "ACPI Reclaimable";
    unsigned char typeACPINVS[] = "ACPI NVS";
    unsigned char typeBadMemory[] = "Bad Memory";
    unsigned char separator[] = "-------------------------\n";
    unsigned char Kbytes[] = " KB";
    unsigned char bytes[] = " bytes\n";

    // Get entry count from 0x8A00 (as you mentioned in your comment)
    uint16_t entries = *(uint16_t *)0x8A00;

    kernel->puts(memMapTitle);
    kernel->puts(entryCount);
    kernel->puti(entries);
    kernel->putc('\n');
    kernel->puts(separator);
      // Define the memory map entry structure according to the documentation
    // Using 32-bit fields for 64-bit values (low and high parts)
    typedef struct {
        uint32_t baseAddrLow;    // Base address low 32 bits
        uint32_t baseAddrHigh;   // Base address high 32 bits
        uint32_t lengthLow;      // Length low 32 bits
        uint32_t lengthHigh;     // Length high 32 bits
        uint32_t type;           // Region type
        uint32_t acpiExt;        // ACPI 3.0 Extended Attributes
    } __attribute__((packed)) MemMapEntry;

    // Entry array starts at 0x8A04 (as per your comment)
    MemMapEntry* memMap = (MemMapEntry*)0x8A04;

    uint32_t totalUsableLow = 0;
    uint32_t totalUsableHigh = 0;
      // Process each entry
    for (uint16_t i = 0; i < entries; i++) {
        // Skip entries with zero length
        if (memMap[i].lengthLow == 0 && memMap[i].lengthHigh == 0) {
            continue;
        }

        // Print entry details
        kernel->puts(baseAddrStr);

        // Display the base address (as two 32-bit values)
        if (memMap[i].baseAddrHigh > 0) {
            kernel->puti(memMap[i].baseAddrHigh);
        }
        kernel->puti(memMap[i].baseAddrLow);
        kernel->putc('\n');

        // Display length
        kernel->puts(lengthStr);
        if (memMap[i].lengthHigh > 0) {
            kernel->puti(memMap[i].lengthHigh);
        }
        kernel->puti(memMap[i].lengthLow);
        kernel->putc('\n');

        // Convert length to KB for display - handling potential overflow
        uint32_t lengthKB;
        if (memMap[i].lengthHigh > 0) {
            // If high part is non-zero, the KB value will be very large
            // Just show the low 32 bits of the KB value
            lengthKB = (memMap[i].lengthLow / 1024) + (memMap[i].lengthHigh * 4194304); // 4194304 = 2^32 / 1024
        } else {
            lengthKB = memMap[i].lengthLow / 1024;
        }
        kernel->putc('(');
        kernel->puti(lengthKB);
        kernel->puts(Kbytes);
        kernel->putc(')');
        kernel->putc('\n');
          // Display memory type
        kernel->puts(typeStr);
        // Make sure we're using a valid index (type 0 is "Unknown")
        if (memMap[i].type >= 1 && memMap[i].type <= 5) {
            if (memMap[i].type == 1) {
                kernel->puts(typeUsable);
            } else if (memMap[i].type == 2) {
                kernel->puts(typeReserved);
            } else if (memMap[i].type == 3) {
                kernel->puts(typeACPIReclaimable);
            } else if (memMap[i].type == 4) {
                kernel->puts(typeACPINVS);
            } else if (memMap[i].type == 5) {
                kernel->puts(typeBadMemory);
            }
        } else {
            kernel->puts(typeUnknown);
        }
        kernel->putc('\n');

        // Keep track of total usable memory
        if (memMap[i].type == 1) { // Type 1 is usable RAM
            // Add the current region to the total, handling potential overflow
            uint32_t newTotalLow = totalUsableLow + memMap[i].lengthLow;
            if (newTotalLow < totalUsableLow) { // Overflow occurred
                totalUsableHigh++;
            }
            totalUsableLow = newTotalLow;
            totalUsableHigh += memMap[i].lengthHigh;
        }

        kernel->puts(separator);
    }
      // Display total usable memory
    unsigned char totalStr[] = "Total Usable Memory: ";
    kernel->puts(totalStr);

    // Convert to KB for display - handling potential overflow
    uint32_t totalKBLow = totalUsableLow / 1024;
    uint32_t overflowBytes = totalUsableLow % 1024;
    uint32_t totalKBHigh = totalUsableHigh * 4194304; // 4194304 = 2^32 / 1024

    // Add in any overflow bytes from the low part
    totalKBHigh += overflowBytes / 1024;

    // Display the result
    if (totalKBHigh > 0) {
        kernel->puti(totalKBHigh);
        kernel->putc(' ');
    }
    kernel->puti(totalKBLow);
    kernel->puts(Kbytes);

    // After displaying the memory map, also show heap information
    print_memory_map();

    // Print memory stats
    print_memory_stats();

    // Main loop in k_entrypoint()
    unsigned char message[] = "Press Ctrl+L to switch keyboard layout, ESC to exit.";
    while (true) {
        // Check for key presses
        if (keyboard_is_key_pressed(KEY_ESCAPE)) {
            break;  // Exit the loop if ESC is pressed
        }
        if (keyboard_is_key_pressed(KEY_LCTRL)) {
            // Switch keyboard layout
            // Here you would implement the logic to switch the keyboard layout
            // For example, you could toggle between different layouts
            // This is just a placeholder for demonstration
        }
        // halt the CPU to save power
        sleep(10000);
    }
}


