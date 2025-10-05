#ifndef IO_H
#define IO_H

static inline int inportb(unsigned int port)
{
    int ret;
    __asm__ __volatile__("inb %%dx,%%al":"=a"(ret):"Nd"(port));
    return ret;
}

static inline void outportb(unsigned int port,unsigned char value)
{
    __asm__ __volatile__("outb %%al,%%dx"::"a"(value),"Nd"(port));
}

static inline void io_wait(void) {
    __asm__ __volatile__("outb %%al, $0x80" : : "a"(0));
}

#endif