echo off
nasm -f bin bootInstructions.asm -o instructions.bin
gcc kernel.c -o kernel.bin -ffreestanding -m32 -fno-pie -nostdlib -nostdinc -fno-builtin -fno-stack-protector -nostartfiles -nodefaultlibs -c -Wno-implicit-function-declaration -mno-sse
objcopy -O binary -j .text kernel.bin kernel.bin
go run ./main.go
bochsdbg -q -f bochs_config.bxrc