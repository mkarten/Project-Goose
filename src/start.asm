; 32-bit entry; NASM, ELF32
BITS 32
global kernel_entry
extern k_init
extern k_entrypoint

section .text.boot align=16
kernel_entry:
    cli
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    mov esp, 0x00050000       ; stack
    mov ebp, 0x00090000       ; optional frame base

    call k_init
    call k_entrypoint

.halt:
    hlt
    jmp .halt
