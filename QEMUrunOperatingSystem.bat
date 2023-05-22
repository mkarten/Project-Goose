echo off
nasm -f bin bootInstructions.asm -o instructions.bin
go run ./main.go
gcc kernel.c -o kernel.bin -ffreestanding -m32 -fno-pie -nostdlib -nostdinc -fno-builtin -fno-stack-protector -nostartfiles -nodefaultlibs -c
qemu-system-x86_64 -drive format=raw,file=boot.bin