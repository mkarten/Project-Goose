[org 0x7c00]
bits 16

entry:
    ; reset all registers to zero
    mov ax,0
    mov ds,ax
    mov es,ax
    mov ss,ax
    mov sp,0x7c00

    ; switch to protected mode
    call EnterProtectedMode


EnterProtectedMode:
    cli ; 1 disable interrupts
    call EnableA20 ; 2 enable A20 line
    call LoadGDT ; 3 load GDT

    ; 4 switch to protected mode
    mov eax, cr0
    or al , 1
    mov cr0, eax
    ; 5 switch to 32-bit code segment
    jmp dword 0x08:ProtectedModeEntry

ProtectedModeEntry:
    ; Start of 32-bit protected mode code

.halt:
    jmp .halt


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

KBCDataPort equ 0x64
KBCCommandPort equ 0x64
KBCDisableKeyboard equ 0xAD
KBCEnableKeyboard equ 0xAE
KBCReadOutputPort equ 0xD0
KBCWriteOutputPort equ 0xD1

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