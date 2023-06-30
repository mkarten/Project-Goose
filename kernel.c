#include "stdint.h"
void _cdecl c_entry_(){
    c_kernel_();
}

// colors
#define White 0x0F
#define Black 0x00
#define Blue 0x01
#define Green 0x02
#define Cyan 0x03
#define Red 0x04
#define Magenta 0x05
#define Brown 0x06
#define LightGray 0x07
#define DarkGray 0x08
#define LightBlue 0x09
#define LightGreen 0x0A
#define LightCyan 0x0B
#define LightRed 0x0C
#define LightMagenta 0x0D
#define Yellow 0x0E

void wait(uint32_t time) {
    for (int i = 0; i < time; i++) {
        __asm__("nop");
    }
}

void putc(char c) {
    struct Kernel *kernel = (struct Kernel *)0x00000000;
    uint16_t *video_memory = (uint16_t *)0xb8000;
    video_memory[kernel->ScreenPosX + kernel->ScreenPosY * kernel->ScreenWidth] = (kernel->BackgroundColor << 12) | (kernel->ForegroundColor << 8) | c;
    kernel->ScreenPosX++;
    if (kernel->ScreenPosX >= kernel->ScreenWidth) {
        kernel->ScreenPosX = 0;
        kernel->ScreenPosY++;
        if (kernel->ScreenPosY >= kernel->ScreenHeight) {
            kernel->ScreenPosY = 0;
        }
    }
}

void clear(uint8_t color) {
    struct Kernel *kernel = (struct Kernel *)0x00000000;
    uint8_t fore_color = kernel->ForegroundColor;
    uint8_t back_color = kernel->BackgroundColor;
    kernel->ForegroundColor = color;
    kernel->BackgroundColor = color;
    for (uint16_t i = 0; i < 80 * 25; i++) {
        kernel->putc(' ');
    }
    kernel->ForegroundColor = fore_color;
    kernel->BackgroundColor = back_color;
    kernel->ScreenPosX = 0;
    kernel->ScreenPosY = 0;
}

void puts(char str[]) {
    struct Kernel *kernel = (struct Kernel *)0x00000000;
    for (uint16_t i = 0; str[i] != '\0'; i++) {
        kernel->putc(str[i]);
    }
}

void init() {
    // store static kernel struct in memory at 0x00000000
    struct Kernel *kernel = (struct Kernel *)0x00000000;

    // initialize kernel variables
    kernel->ScreenWidth = 80;
    kernel->ScreenHeight = 25;
    kernel->ScreenPosX = 0;
    kernel->ScreenPosY = 0;
    kernel->ForegroundColor = 0x0B;
    kernel->BackgroundColor = 0x00;


    // initialize kernel functions
    kernel->putc = putc+0x7e00;
    kernel->clear = clear+0x7e00;
    kernel->wait = wait+0x7e00;
    kernel->puts = puts+0x7e00;
}

void _cdecl c_kernel_() {
    init();
    struct Kernel *kernel = (struct Kernel *)0x00000000;
    // test kernel functions
    uint8_t  color = 0x00;
    while (1==1) {
        kernel->clear(color);
        color++;
        if (color > 0x0F) {
            color = 0x00;
        }
        kernel->wait(0x1000000);
    }

    char str[] = "Hello, World!";
    kernel->puts(str);
}

