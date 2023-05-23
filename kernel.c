#include "stdint.h"

void _cdecl c_entry_() {
    uint16_t *video_memory = (uint16_t *)0xb8000;
    uint8_t fore_color = 0x0A;
    uint8_t back_color = 0x00;
    char str[] = "Hello From C!";


    c_clear_screen(video_memory, fore_color, back_color);
    c_puts(video_memory, fore_color, back_color, str, 0);

}

void _cdecl c_wait() {
    for (int i = 0; i < 500; i++) {
        __asm__("nop");
    }
}

void _cdecl c_putc(uint16_t *vmem, uint8_t fore_color, uint8_t back_color, char c, uint16_t pos) {
    vmem[pos] = (back_color << 12) | (fore_color << 8) | c;
    c_wait();
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