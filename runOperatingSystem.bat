echo off
nasm -f bin bootInstructions.asm -o instructions.bin
go run ./main.go
qemu-system-x86_64 boot.bin