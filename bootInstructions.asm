[org 0x7c00]
bits 16

entry:
    ; load the disk number of the boot drive
    mov dl,DiskNumber
    ; reset all registers to zero
    mov ax,0
    mov ds,ax
    mov es,ax
    mov ss,ax
    mov sp,StackStart
    mov bp,sp

    ; print the message "Hello, World"
    mov si, TestMessage
    call PrintString


    ; print the message "Loading CB bootloader..."
    mov si, StartLoadingCBootloader
    call PrintString

    ; load the bootloader to 0x7e00
    call LoadCBootloader

    ; print the message "CB bootloader loaded successfully"
    mov si, SuccessLoadingCBootloader
    call PrintString

    ; switch to protected mode
    call EnterProtectedMode

PrintString:
    pusha
.loop:
    mov ah, 0x0e
    mov bh, 0x00
    mov bl, 0x07
    mov al, [si]
    cmp al, 0
    je .done
    int 0x10
    inc si
    jmp .loop
.done:
    popa
    ret

LoadCBootloader:
    mov ah, 0x02 ; read sectors from disk
    mov al, 4 ; read 4 sectors
    mov ch, 0 ; cylinder 0
    mov cl, 2 ; sector 2
    mov dh, 0 ; head 0
    mov dl, DiskNumber ; boot drive
    mov bx, CBootloaderEntry ; load to 0x7e00
    int 0x13 ; BIOS interrupt
    jc .diskError ; check if error
    cmp al, 4 ; check if 4 sectors were read
    jne .sectorError ; if not, print error message
    ret

.diskError:
    ; Print the error message
    mov si, DriveError
    call PrintString
    jmp .halt

.sectorError:
    ; Print the error message
    mov si, SectorError
    call PrintString
    jmp .halt

.halt:
    jmp .halt

EnterProtectedMode:
    cli ; 1 disable interrupts
    call EnableA20 ; 2 enable A20 line
    call LoadGDT ; 3 load GDT

    ; 4 switch to protected mode
    mov eax, cr0
    or al , 1
    mov cr0, eax
    ; 5 jump to 32-bit code segment
    jmp dword 0x08:ProtectedModeEntry


ProtectedModeEntry:
bits 32
    ; Start of 32-bit protected mode code
    ; load first byte of CBootloaderEntry to eax
    mov eax, [CBootloaderEntry+0xDC]
    ; output the value to video memory
    mov word [0xb8000], ax
    mov byte [0xb8001], 0x1f ; white on blue
    inc eax
    mov word [0xb8002], ax
    mov byte [0xb8003], 0x1f ; white on blue
    call CBootloaderEntry +0xB4
    ; End of 32-bit protected mode code
    ; Infinite loop
.halt:
    jmp .halt


bits 16
halt:
    jmp halt

EnableA20:
    ; check if A20 line is already enabled
    call A20WaitInput
    mov al, KBCReadOutputPort
    out KBCCommandPort, al

    call A20WaitOutput
    in al, KBCDataPort
    test al, 0x02 ; check bit 2
    jnz A20AlreadyEnabled

    ; disable keyboard
    call A20WaitInput
    mov al, KBCDisableKeyboard
    out KBCCommandPort, al

    ; read output port
    call A20WaitInput
    mov al, KBCReadOutputPort
    out KBCCommandPort, al

    call A20WaitOutput
    in al, KBCDataPort
    push eax

    ; set bit 2 (A20 line) to 1
    call A20WaitInput
    mov al, KBCWriteOutputPort
    out KBCCommandPort, al

    call A20WaitInput
    pop eax
    or al, 0x02 ; set bit 2 to 1
    out KBCDataPort, al

    ; enable keyboard
    call A20WaitInput
    mov al, KBCEnableKeyboard
    out KBCCommandPort, al
    ret

A20AlreadyEnabled:
    call A20WaitInput
    ret

A20WaitInput:
    ; wait until status bit 2 is 0
    in al, KBCCommandPort
    test al, 0x02
    jnz A20WaitInput
    ret

A20WaitOutput:
    ; wait until status but 1 is 1 in order to be read
    in al, KBCCommandPort
    test al, 0x01
    jz A20WaitOutput
    ret

LoadGDT:
    ; load GDT
    lgdt [g_GDTDesc]
    ret


; Keyboard controller ports
KBCDataPort equ 0x64
KBCCommandPort equ 0x64
KBCDisableKeyboard equ 0xAD
KBCEnableKeyboard equ 0xAE
KBCReadOutputPort equ 0xD0
KBCWriteOutputPort equ 0xD1

; General purpose variables
StackStart equ 0x7b00
CBootloaderEntry equ 0x7e00
DiskNumber equ 0x80
ErrorCounterPtr equ 0x7bfe

; Standard messages
TestMessage:
    db "Hello, World", 0x0d, 0x0a, 0
StartLoadingCBootloader:
    db "Loading C bootloader...", 0x0d, 0x0a, 0

; Success messages
SuccessLoadingCBootloader:
    db "C bootloader loaded successfully", 0x0d, 0x0a, 0

; Error messages
DriveError:
    db "Error reading drive", 0x0d, 0x0a, 0
SectorError:
    db "number of sectors read is not equal to number of sectors requested", 0x0d, 0x0a, 0

g_GDT: 
    ; Null descriptor
    dq 0

    ; 32-bit code segment
    dw 0xffff ; limit
    dw 0x0000 ; base
    db 0x00 ; base
    db 10011010b ; access (present, ring 0, code, executable, direction 0 (conforming), readable)
    db 11001111b ; granularity (4k pages, 32-bit protected mode) + limit
    db 0x00 ; base

    ; 32-bit data segment
    dw 0xffff ; limit
    dw 0x0000 ; base
    db 0x00 ; base
    db 10010010b ; access (present, ring 0, data, direction 0 (expand down), writable)
    db 11001111b ; granularity (4k pages, 32-bit protected mode) + limit
    db 0x00 ; base

    ; 16-bit code segment
    dw 0xffff ; limit
    dw 0x0000 ; base
    db 0x00 ; base
    db 10011010b ; access (present, ring 0, code, executable, direction 0 (conforming), readable)
    db 00001111b ; granularity (1 byte pages, 16-bit protected mode) + limit
    db 0x00 ; base

    ; 16-bit data segment
    dw 0xffff ; limit
    dw 0x0000 ; base
    db 0x00 ; base
    db 10010010b ; access (present, ring 0, data, direction 0 (expand down), writable)
    db 00001111b ; granularity (1 byte pages, 16-bit protected mode) + limit
    db 0x00 ; base


g_GDTDesc : dw g_GDTDesc - g_GDT - 1 ; size of GDT
            dw g_GDT ; address of GDT


times 510-($-$$) db 0
dw 0AA55h