#include "stdint.h"

void _cdecl c_entry_() {
    uint16_t *video_memory = (uint16_t *)0xb8000;
    c_putc(video_memory, 0x0B, 0x00, '1', 0);
    for (int i = 0; i < 100000; i++) {
        __asm__("nop");
    }
    uint8_t fore_color = 0x0B;
    c_putc(video_memory, fore_color, 0x00, '2', 1);
    for (int i = 0; i < 100000; i++) {
        __asm__("nop");
    }
    uint8_t back_color = 0x00;
    c_putc(video_memory, fore_color, back_color, '3', 2);
    for (int i = 0; i < 100000; i++) {
        __asm__("nop");
    }
    __asm__("nop");
    __asm__("nop");
    __asm__("nop");
    __asm__("nop");
    __asm__("nop");
    char str[] = "Hello from MAMAMAMMAMAMAMMAMMMAMMAMA!";

    c_clear_screen(video_memory, fore_color, back_color);
    c_puts(video_memory, fore_color, back_color, str, 0);

}

void _cdecl c_wait(uint32_t time) {
    for (int i = 0; i < time; i++) {
        __asm__("nop");
    }
}

void _cdecl c_putc(uint16_t *vmem, uint8_t fore_color, uint8_t back_color, char c, uint16_t pos) {
    vmem[pos] = (back_color << 12) | (fore_color << 8) | c;
    c_wait(100);
    c_wait(100);
}

void _cdecl c_clear_screen(uint16_t *vmem, uint8_t fore_color, uint8_t back_color) {
    for (uint16_t i = 0; i < 80 * 25; i++) {
        c_putc(vmem, fore_color, back_color, ' ', i);
    }
}

void _cdecl c_puts(uint16_t *vmem, uint8_t fore_color, uint8_t back_color, char *str, uint16_t pos) {
    for (uint16_t i = 0; str[i] != '\0'; i++) {
        c_putc(vmem, fore_color, back_color, str[i], pos + i);
    }
}