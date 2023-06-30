[org 0x7c00]
bits 16

entry:

    ; reset all registers to zero
    cli
	xor ax, ax
	mov ds,ax
	mov es,ax
	mov fs,ax
	mov gs,ax
	mov ss,ax
	mov sp,0xffff
	sti

	mov BYTE[BootDrive], dl

    ; print the message "Loading CB bootloader..."
    mov si, StartLoadingCBootloader
    call PrintString

    ; load the bootloader to 0x7e00
    call LoadCBootloader

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
    xor ax, ax
    mov ds , ax
    mov ah, 0x02 ; read sectors from disk
    mov al, 2 ; read 4 sectors
    mov ch, 0 ; cylinder 0
    mov cl, 2 ; sector 2
    mov dh, 0 ; head 0
    mov dl, [BootDrive] ; boot drive
    mov bx, 0x7e00 ; load to 0x7e00
    int 0x13 ; BIOS interrupt
    ; print content of AH register
    pushf
    push ax
    mov ah, 0x0e
    add al, 0x30
    int 0x10
    pop ax
    popf
    ; check for errors
    jc .diskError ; check if error
    cmp al, 2 ; check if 4 sectors were read
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

    mov si, EnterProtectedModeMessage
    call PrintString

    ; 4 switch to protected mode
    mov eax, cr0
    or al , 1
    mov cr0, eax
    ; load 32-bit code data segment
    mov ax, 0x10
    mov ds, ax
    ; 5 jump to 32-bit code segment
    jmp dword 0x08:0x7e00


ProtectedModeEntry:
bits 32
    ; Start of 32-bit protected mode code
    ;
    call CBootloaderEntry
    ; End of 32-bit protected mode code
    ; Infinite loop
.halt:
    jmp .halt


bits 16
halt:
    jmp halt

EnableA20:
    mov     ax,2403h                ;--- A20-Gate Support ---
    int     15h
    jb      a20_ns                  ;INT 15h is not supported
    cmp     ah,0
    jnz     a20_ns                  ;INT 15h is not supported

    mov     ax,2402h                ;--- A20-Gate Status ---
    int     15h
    jb      a20_failed              ;couldn't get status
    cmp     ah,0
    jnz     a20_failed              ;couldn't get status

    cmp     al,1
    jz      a20_activated           ;A20 is already activated

    mov     ax,2401h                ;--- A20-Gate Activate ---
    int     15h
    jb      a20_failed              ;couldn't activate the gate
    cmp     ah,0
    jnz     a20_failed              ;couldn't activate the gate

    a20_activated:                  ;go on
    mov si, A20ActivatedMessage
    call PrintString
    ret

    a20_ns:
    mov si, Int15hNotSupportedMessage
    call PrintString
    .halt:
    jmp .halt

    a20_failed:
    mov si, A20FailedMessage
    call PrintString
    .halt:
    jmp .halt

LoadGDT:
    ; load GDT
    lgdt [g_GDTDesc]
    ; print the message "Loaded GDT"
    mov si, LoadGDTMessage
    call PrintString
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
BootDrive:
    db 0
ErrorCounterPtr equ 0x7bfe

; Standard messages
StartLoadingCBootloader:
    db "Loading C bootloader...", 0x0d, 0x0a, 0

LoadGDTMessage:
    db "Loaded GDT", 0x0d, 0x0a, 0

EnterProtectedModeMessage:
    db "Entering protected mode...", 0x0d, 0x0a, 0

Int15hNotSupportedMessage:
    db "INT 15h is not supported", 0x0d, 0x0a, 0

A20ActivatedMessage:
    db "A20 is activated", 0x0d, 0x0a, 0

A20FailedMessage:
    db "Couldn't activate A20", 0x0d, 0x0a, 0

; Success messages

; Error messages
DriveError:
    db "Error reading drive", 0x0d, 0x0a, 0
SectorError:
    db "read is not = to sectors requested", 0x0d, 0x0a, 0

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