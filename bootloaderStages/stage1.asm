; Stage 1 Bootloader (Boot Sector)
; Assembled with NASM: nasm -f bin -o stage1.bin stage1.asm

[org 0x7c00]
bits 16

start:
    ; Store boot drive number before initializing segments
    mov BYTE[BootDrive], dl

    ; Initialize segments and stack
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0xFFFF
    sti

    ; Display loading message
    mov si, msg_loading
    call PrintString

    ; Load Stage 2 Bootloader
    mov ah, 0x02          ; BIOS Read Sectors function
    mov al, 16            ; Number of sectors for Stage 2 (matching its size)
    mov ch, 0             ; Cylinder number
    mov cl, 2             ; Sector number (start after boot sector)
    mov dh, 0             ; Head number
    mov dl, [BootDrive]   ; Boot drive number
    mov bx, 0x7F00 ; Buffer to load Stage 2 Bootloader
    int 0x13              ; BIOS Disk Service
    jc disk_error         ; Jump if carry flag set (error)

; Clear registers before jumping to Stage 2
    xor ax, ax
    xor bx, bx
    xor cx, cx
    xor dx, dx
    xor si, si
    xor di, di
    xor bp, bp
    xor sp, sp

; put the boot drive number back in dl
    mov dl, [BootDrive]

    jmp 0x7F00 ; Jump to Stage 2 Bootloader

disk_error:
    ; Display disk error message
    mov si, msg_disk_error
    call PrintString

    ; Infinite loop to halt execution
.halt:
    cli
    hlt                   ; Halt the CPU
    jmp .halt            ; Infinite loop

; Subroutine to print string using BIOS
PrintString:
    mov ah, 0x0E          ; BIOS Teletype Output function
.print_loop:
    lodsb                 ; Load byte at DS:SI into AL and increment SI
    or al, al
    jz .done
    int 0x10              ; BIOS Video Service
    jmp .print_loop
.done:
    ret

BootDrive       db 0

msg_loading     db "Loading Stage 2 Bootloader...", 0
msg_disk_error  db "Disk read error!", 0

times 510-($-$$) db 1
dw 0xAA55                   ; Boot signature
