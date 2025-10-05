/* 32-bit flat kernel linked to run at 0x0000A000,
   but emitted as a compact flat binary starting at file offset 0. */
ENTRY(kernel_entry)

KLOAD = 0x0000A000;       /* where stage2 loads it in RAM */
PHYS0 = 0x00000000;       /* file offset base */

SECTIONS
{
  /* Virtual addresses (where code THINKS it lives) */
  . = KLOAD;

  /* Code */
    .text : AT(ADDR(.text)-KLOAD) {
        KEEP(*(.text.boot))    /* start.S lands here first */
        *(.text .text.*)
    }

  /* Read-only data */
  .rodata : AT(ADDR(.rodata) - KLOAD) {
    *(.rodata .rodata.*)
  }

  /* Writable data */
  .data : AT(ADDR(.data) - KLOAD) {
    *(.data .data.*)
  }

  /* Zero-initialized, occupies space in memory but not in file */
  .bss (NOLOAD) : {
    *(COMMON)
    *(.bss .bss.*)
  }
}
