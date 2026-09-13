OUTPUT_FORMAT(elf32-sh)
OUTPUT_ARCH(sh3)
ENTRY(_usb_poc_start)
MEMORY { rom (rx) : ORIGIN = 0x00300000, LENGTH = 64K }
SECTIONS {
  .text : { *(.pretext) *(.text*) } > rom
  .rodata : ALIGN(4) { *(.rodata*) } > rom
  .data : { *(.data*) } > rom
  .bss (NOLOAD) : { *(.bss*) *(COMMON) } > rom
  ASSERT(SIZEOF(.data) == 0 && SIZEOF(.bss) == 0, "USB PoC requires no mutable globals")
}
