[BITS 32]
global idt_load
global keyboard_handler_wrapper

extern keyboard_handler   ; Keyboard handler in C

idt_load:
    mov eax, [esp+4]     ; Get pointer to IDT
    lidt [eax]          ; Load IDT
    ret

keyboard_handler_wrapper:
    pushad              ; Push all general purpose registers
    
    call keyboard_handler
    
    mov al, 0x20       ; Send EOI to PIC
    out 0x20, al
    
    popad              ; Restore all registers
    iret               ; Return from interrupt