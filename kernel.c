#include "stdint.h"
#include "kernelStruct.h"
#include "colors.h"

void _cdecl c_entry_(){
    c_kernel_();
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

void wait(uint32_t time) {
    for (int i = 0; i < time; i++) {
        __asm__("nop");
    }
}

void putc(unsigned char c) {
    struct Kernel *kernel = (struct Kernel *)0x00000000;
    uint16_t *video_memory = (uint16_t *)0xb8000;
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
        video_memory[kernel->ScreenPosX + kernel->ScreenPosY * kernel->ScreenWidth] = (kernel->BackgroundColor << 12) | (kernel->ForegroundColor << 8) | ' ';
    } else {
        video_memory[kernel->ScreenPosX + kernel->ScreenPosY * kernel->ScreenWidth] = (kernel->BackgroundColor << 12) | (kernel->ForegroundColor << 8) | c;
        kernel->ScreenPosX++;
    }
    if (kernel->ScreenPosX >= kernel->ScreenWidth) {
        kernel->ScreenPosX = 0;
        kernel->ScreenPosY++;
        if (kernel->ScreenPosY >= kernel->ScreenHeight) {
            // scroll screen one line up
            for (uint16_t y = 0; y < kernel->ScreenHeight - 1; y++) {
                for (uint16_t x = 0; x < kernel->ScreenWidth; x++) {
                    video_memory[x + y * kernel->ScreenWidth] = video_memory[x + (y + 1) * kernel->ScreenWidth];
                }
            }

        }
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

void init() {
    // store static kernel struct in memory at 0x00000000
    struct Kernel *kernel = (struct Kernel *)0x00000000;

    // initialize kernel variables
    kernel->ScreenWidth = 80;
    kernel->ScreenHeight = 25;
    kernel->ScreenPosX = 0;
    kernel->ScreenPosY = 0;
    kernel->ForegroundColor = 0x07;
    kernel->BackgroundColor = 0x00;


    // initialize kernel functions
    kernel->putc = putc+0x7e00;
    kernel->clear = clear+0x7e00;
    kernel->wait = wait+0x7e00;
    kernel->puts = puts+0x7e00;
    kernel->puti = puti+0x7e00;
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

void _cdecl c_kernel_() {
    init();
    unsigned char Klayout[128] =
    {
        0,0,'1','2','3','4','5','6','7','8','9','0','-','=','\b','\t','q','w','e','r','t','y','u','i','o','p','[',']','\n',        0,'a','s','d','f','g','h','j','k','l',';','\'','`',0,'\\','z','x','c','v','b','n','m',',','.','/',0,'*',0,' ',
    };
    struct Kernel *kernel = (struct Kernel *)0x00000000;
    // test kernel functions
    uint8_t  color = Black;
//    while (1==1) {
//        kernel->clear(color);
//        color++;
//        if (color > 0x0F) {
//            color = 0x00;
//        }
//    }
    kernel->clear(Black);
    enable_cursor(15, 15);
    unsigned char str[] = "Hello, World!\n";
    kernel->puts(str);
    restart_keyboard();
    while(1==1){
        unsigned char scancode = get_scancode();
        // check if scancode is a key release
        if(scancode & 0x80){
            // do nothing
        }else{
            // print value of scancode
            kernel->putc(Klayout[scancode]);
        }
     }
}


