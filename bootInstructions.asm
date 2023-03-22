[org 0x7c00]
mov ah,0x0e
mov bx,string

printString:
    mov al,[bx]
    cmp al,0
    je end
    int 0x10
    inc bx
    jmp printString
end:
    jmp $

string:
    db 10,32,32,32,32,"Hello from Project-Goose",0