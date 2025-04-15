echo off
for /r bootloaderStages %%f in (*.asm) do nasm -f bin %%f -o bootloaderStages/%%~nf.bin
gcc kernel.c -o kernel.bin -ffreestanding -m32 -c -Wno-implicit-function-declaration -mno-sse
objcopy -O binary -j .text kernel.bin kernel.bin
go run ./main.go
bochsdbg -q -f bochs_config.bxrc