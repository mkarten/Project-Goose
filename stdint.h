#pragma once

typedef signed char int8_t;
typedef unsigned char uint8_t;
typedef signed short int16_t;
typedef unsigned short uint16_t;
typedef signed int int32_t;
typedef unsigned int uint32_t;

typedef void (*putc_t)(char c);
typedef void (*clear_t)(uint8_t color);
typedef void (*wait_t)(uint32_t time);
typedef void (*puts_t)(char str[]);


struct Kernel{
    uint32_t ScreenWidth;
    uint32_t ScreenHeight;
    uint32_t ScreenPosX;
    uint32_t ScreenPosY;
    uint8_t  ForegroundColor;
    uint8_t  BackgroundColor;

    putc_t putc;
    clear_t clear;
    wait_t wait;
    puts_t puts;
};

