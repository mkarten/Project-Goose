typedef void (*putc_t)(unsigned char c);
typedef void (*clear_t)(uint8_t color);
typedef void (*sleep_t)(uint32_t time);
typedef void (*puts_t)(unsigned char str[]);
typedef void (*puti_t)(int32_t num);


struct Kernel{
    uint32_t ScreenWidth;
    uint32_t ScreenHeight;
    uint32_t ScreenPosX;
    uint32_t ScreenPosY;
    uint8_t  ForegroundColor;
    uint8_t  BackgroundColor;
    putc_t putc;
    clear_t clear;
    sleep_t sleep;
    puts_t puts;
    puti_t puti;
    char     KeyboardLayout[128];
};