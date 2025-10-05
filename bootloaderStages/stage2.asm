; Stage 2 Bootloader
; Assembled with NASM: nasm -f bin -o stage2.bin stage2.asm

[org 0x7F00]
bits 16

start:
    ; get the boot drive number
    mov dl, [BootDrive]

    ; Initialize registers and segments
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0xFFFF

    ; Display message
    mov si, msg_stage2_loaded
    call PrintString

    ; Get Available Memory using E820h
    call GetMemoryMap

    in al, 0x92            ; Read port 0x92
    or al, 0x02            ; Set bit 1 (A20 gate enable)
    out 0x92, al           ; Write back to port 0x92

    ; Load Kernel into memory
    call LoadKernel

    ; Load GDT and switch to protected mode
    call EnterProtectedMode

    ; Infinite loop
.hang:
    cli
    hlt
    jmp .hang

; Subroutine to print string using BIOS
PrintString:
    push ax
    mov ah, 0x0E          ; BIOS Teletype Output function
.print_loop:
    lodsb
    or al, al
    jz .done
    int 0x10              ; BIOS Video Service
    jmp .print_loop
.done:
    pop ax
    ret

PrintNumber:
    ; Print a number in EAX
    pusha
    mov ecx, 10           ; Base 10
    xor ebx, ebx          ; Clear EBX for division
.next_digit:
    xor edx, edx          ; Clear EDX before division
    div ecx               ; EAX / 10, quotient in EAX, remainder in EDX
    push dx               ; Push remainder (digit) onto stack
    test eax, eax         ; Check if quotient is zero
    jnz .next_digit       ; If not, continue dividing
.print_loop:
    pop dx                ; Pop digit from stack
    add dl, '0'           ; Convert to ASCII
    mov ah, 0x0E          ; BIOS Teletype Output function
    int 0x10              ; Print character
    test eax, eax         ; Check if quotient is zero
    jnz .print_loop       ; If not, continue printing
    popa
    ret

LoadKernel:
    ; reset the disk controller
    xor ax, ax
    mov dl, [BootDrive]   ; Boot drive number
    mov ah, 0x00          ; BIOS Reset Disk Controller function
    int 0x13              ; BIOS Disk Service
    jc .error             ; If CF set, error


    ; Load Kernel into memory
    mov ah, 0x02          ; BIOS Read Sector function
    mov al, 22             ; Read 8 sectors (matching kernel size)
    mov ch, 0             ; Cylinder 0
    mov dh, 0             ; Head 0
    mov cl, 18            ; Sector 11
    mov bx, 0xA000        ; Buffer to load kernel
    int 0x13              ; BIOS Disk Service
    jc .error             ; If CF set, error
    ret
.error:
    ; Display error message
    mov si, msg_error
    call PrintString
    jmp .hang
.hang:
    cli
    hlt
    jmp .hang

; Subroutine to get memory map using INT 15h, E820h
mmap_ent equ 0x8A00             ; the number of entries will be stored at 0x8000
GetMemoryMap:
    mov di, 0x8A04          ; Set di to 0x8004. Otherwise this code will get stuck in `int 0x15` after some entries are fetched
    xor ebx, ebx           ; ebx must be 0 to start
    xor bp, bp             ; keep an entry count in bp
    mov edx, 0x0534D4150   ; Place "SMAP" into edx
    mov eax, 0xe820
    mov [es:di + 20], dword 1 ; force a valid ACPI 3.X entry
    mov ecx, 24            ; ask for 24 bytes
    int 0x15
    jc short .failed       ; carry set on first call means "unsupported function"
    mov edx, 0x0534D4150   ; Some BIOSes apparently trash this register?
    cmp eax, edx           ; on success, eax must have been reset to "SMAP"
    jne short .failed
    test ebx, ebx          ; ebx = 0 implies list is only 1 entry long (worthless)
    je short .failed
    jmp short .jmpin
.e820lp:
    mov eax, 0xe820        ; eax, ecx get trashed on every int 0x15 call
    mov [es:di + 20], dword 1 ; force a valid ACPI 3.X entry
    mov ecx, 24            ; ask for 24 bytes again
    int 0x15
    jc short .e820f        ; carry set means "end of list already reached"
    mov edx, 0x0534D4150   ; repair potentially trashed register
.jmpin:
    jcxz .skipent          ; skip any 0 length entries
    cmp cl, 20             ; got a 24 byte ACPI 3.X response?
    jbe short .notext
    test byte [es:di + 20], 1 ; if so: is the "ignore this data" bit clear?
    je short .skipent
.notext:
    mov ecx, [es:di + 8]   ; get lower uint32_t of memory region length
    or ecx, [es:di + 12]   ; "or" it with upper uint32_t to test for zero
    jz .skipent            ; if length uint64_t is 0, skip entry
    inc bp                 ; got a good entry: ++count, move to next storage spot
    add di, 24
.skipent:
    test ebx, ebx          ; if ebx resets to 0, list is complete
    jne short .e820lp
.e820f:
    mov [es:mmap_ent], bp  ; store the entry count
    clc                    ; there is "jc" on end of list to this point, so the carry must be cleared
    ret
.failed:
    stc                    ; "function unsupported" error exit
    ret

.display_loop:
    ; Print each memory map entry (20 bytes per entry)
    mov ah, 0x0E
    lodsb
    or al, al
    jz .done_display
    int 0x10
    loop .display_loop
.done_display:
    popa
    ret

; Subroutine to load GDT and enter protected mode
EnterProtectedMode:
    cli
    call setGdt
    mov eax, cr0
    or eax, 1              ; Set PE bit to enable protected mode
    mov cr0, eax

    ; load 32-bit code segment
    mov ax, 0x10
    mov ds, ax

    jmp 0x08:0xA000 ; Far jump to 32-bit code segment
.hang:
    hlt
    jmp .hang

bits 16
; Data definitions
BootDrive       db 0
MemCount        dw 0    ; Number of memory map entries
MemoryMapBuffer times 1024 db 0  ; Buffer to store memory map entries


msg_stage2_loaded db "Stage 2 Bootloader Loaded.", 0
msg_error        db "Error loading kernel.", 0

; GDT setup
GDT:
    ; Null descriptor
    dq 0

    ; 32-bit code segment descriptor
    dw 0xFFFF          ; Limit low
    dw 0x0000          ; Base low
    db 0x00            ; Base middle
    db 0x9A            ; Access byte (present, ring 0, executable, readable)
    db 0xCF            ; Flags and limit high (4 KB granularity, 32-bit segment)
    db 0x00            ; Base high

    ; 32-bit data segment descriptor
    dw 0xFFFF          ; Limit low
    dw 0x0000          ; Base low
    db 0x00            ; Base middle
    db 0x92            ; Access byte (present, ring 0, writable)
    db 0xCF            ; Flags and limit high (4 KB granularity, 32-bit segment)
    db 0x00            ; Base high

    ; 16-bit code segment descriptor
    dw 0xFFFF          ; Limit low
    dw 0x0000          ; Base low
    db 0x00            ; Base middle
    db 0x9A            ; Access byte (present, ring 0, executable, readable)
    db 0x0F            ; Flags and limit high (1-byte granularity, 16-bit segment)
    db 0x00            ; Base high

    ; 16-bit data segment descriptor
    dw 0xFFFF          ; Limit low
    dw 0x0000          ; Base low
    db 0x00            ; Base middle
    db 0x92            ; Access byte (present, ring 0, writable)
    db 0x0F            ; Flags and limit high (1-byte granularity, 16-bit segment)
    db 0x00            ; Base high

GDT_end:

gdtr DW 0 ; For limit storage
     DD 0 ; For base storage

setGdt:
   XOR   EAX, EAX
   MOV   AX, DS
   SHL   EAX, 4
   ADD   EAX, GDT
   MOV   [gdtr + 2], eax
   MOV   EAX, GDT_end
   SUB   EAX, GDT
   MOV   [gdtr], AX
   LGDT  [gdtr]
   RET

; Pad to 8192 bytes
times 8192 - ($ - $$) db 2
