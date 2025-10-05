@echo off
setlocal enabledelayedexpansion

REM ============================================
REM Toolchain autodetect (LLVM -> GNU cross)
REM ============================================
set "CC="
set "LD="
set "OBJCOPY="
set "TOOLCHAIN="

where clang    >nul 2>nul && where ld.lld       >nul 2>nul && where llvm-objcopy >nul 2>nul
if %errorlevel%==0 (
  set "CC=clang"
  set "LD=ld.lld"
  set "OBJCOPY=llvm-objcopy"
  set "TOOLCHAIN=LLVM (ELF)"
) else (
  where i686-elf-gcc    >nul 2>nul && where i686-elf-ld     >nul 2>nul && where i686-elf-objcopy >nul 2>nul
  if %errorlevel%==0 (
    set "CC=i686-elf-gcc"
    set "LD=i686-elf-ld"
    set "OBJCOPY=i686-elf-objcopy"
    set "TOOLCHAIN=GNU i686-elf (ELF)"
  )
)

if not defined CC (
  echo.
  echo [ERROR] No ELF-capable toolchain found.
  echo         Install LLVM or a GNU i686-elf cross toolchain.
  echo.
  exit /b 1
)

where nasm     >nul 2>nul || echo [WARN] NASM not found on PATH.
where go       >nul 2>nul || echo [WARN] Go not found on PATH.
where bochsdbg >nul 2>nul || echo [WARN] bochsdbg not found on PATH.

echo Using toolchain: %TOOLCHAIN%
echo CC=%CC%
echo LD=%LD%
echo OBJCOPY=%OBJCOPY%
echo.

REM ============================================
REM Project config
REM ============================================
set "SRC_DIR=src"
set "INC_DIR=includes"
set "BUILD_DIR=build"
set "OUT_ELF=kernel.elf"
set "OUT_BIN=kernel.bin"
set "LINKER=linker.ld"

REM ============================================
REM Flags
REM ============================================
if /I "%TOOLCHAIN%"=="LLVM (ELF)" (
  set "CFLAGS=-target i686-elf -std=gnu11 -ffreestanding -fno-builtin -fno-stack-protector -m32 -fno-pic -mno-sse -mno-sse2 -Wall -Wextra -Wundef -Wshadow -I %INC_DIR%"
  set "ASFLAGS=%CFLAGS%"
  set "LDFLAGS=-m elf_i386 -T %LINKER% -nostdlib -Map %BUILD_DIR%\kernel.map"
) else (
  set "CFLAGS=-std=gnu11 -ffreestanding -fno-builtin -fno-stack-protector -m32 -fno-pic -mno-sse -mno-sse2 -Wall -Wextra -Wundef -Wshadow -I %INC_DIR%"
  set "ASFLAGS=%CFLAGS%"
  set "LDFLAGS=-m elf_i386 -T %LINKER% -nostdlib -Map %BUILD_DIR%\kernel.map"
)

REM ============================================
REM Build
REM ============================================
echo === Clean ===
if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"
del /q "%BUILD_DIR%\*.o" "%OUT_ELF%" "%OUT_BIN%" 2>nul

echo === Assemble bootloader stages (.asm -> .bin) ===
for /r "bootloaderStages" %%f in (*.asm) do (
  echo [NASM BIN] %%f
  echo nasm -f bin "%%~ff" -o "bootloaderStages\%%~nf.bin"
  nasm -f bin "%%~ff" -o "bootloaderStages\%%~nf.bin"
  if errorlevel 1 goto :error
)

echo === Compile C sources (src\*.c) ===
for /r "%SRC_DIR%" %%f in (*.c) do (
  echo [CC] %%f
  "%CC%" %CFLAGS% -c "%%~ff" -o "%BUILD_DIR%\%%~nf.o"
  if errorlevel 1 goto :error
)

echo === Assemble kernel NASM (src\*.asm -> .o ELF32) ===
for /r "%SRC_DIR%" %%f in (*.asm) do (
  echo [NASM ELF32] %%f
  echo nasm -f elf32 -I "%INC_DIR%" -I "%SRC_DIR%" "%%~ff" -o "%BUILD_DIR%\%%~nf.o"
  nasm -f elf32 -I "%INC_DIR%" -I "%SRC_DIR%" "%%~ff" -o "%BUILD_DIR%\%%~nf.o"
  if errorlevel 1 goto :error
)

echo === Assemble preprocessed ASM (src\*.S -> .o) if any ===
for /r "%SRC_DIR%" %%f in (*.S) do (
  echo [AS] %%f
  "%CC%" %ASFLAGS% -c "%%~ff" -o "%BUILD_DIR%\%%~nf.o"
  if errorlevel 1 goto :error
)

echo === Link ===
"%LD%" %LDFLAGS% "%BUILD_DIR%\*.o" -o "%OUT_ELF%"
if errorlevel 1 goto :error

echo === Objcopy: ELF -> flat binary ===
"%OBJCOPY%" -O binary "%OUT_ELF%" "%OUT_BIN%"
if errorlevel 1 goto :error

REM --- Report size & sectors (512B) ---
for %%A in ("%OUT_BIN%") do set "KSIZE=%%~zA"
set /a KSECTORS=(KSIZE + 511) / 512
echo Kernel size: %KSIZE% bytes  ^(~%KSECTORS% sectors^)

echo === Go step (e.g., write kernel at CHS 0/0/18, count=%KSECTORS%) ===
set "KERNEL_SECTORS=%KSECTORS%"
set "KERNEL_START_C=0"
set "KERNEL_START_H=0"
set "KERNEL_START_S=18"

go run ".\main.go"
if errorlevel 1 goto :error

echo === Bochs ===
bochsdbg -q -f "bochs_config.bxrc"
goto :eof

:error
echo.
echo Build failed.
exit /b 1
