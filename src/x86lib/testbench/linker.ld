OUTPUT_FORMAT("elf32-i386")
ENTRY(start)
phys = 0x1008;
scratch = 0x100000;
MEMORY
{
  coderom (rx) : ORIGIN = phys, LENGTH = 128K
  scratchram (!rx) : ORIGIN = scratch, LENGTH = 1024K
}
SECTIONS
{
  . = ALIGN(4);
  .text : AT(phys) ALIGN(4) {
    . = ALIGN(4);
    code = .;
    *(.text.start);
    *(.text*)
    *(.rodata)
    . = ALIGN(4);
  }
  __text_end=.;
  . = ALIGN(4);
  .data : AT(scratch)
  {
    . = ALIGN(4);
    data = .;
    *(.data)
    . = ALIGN(4);
  } > scratchram
  __data_end=.;
  __binary_end = .;
  .bss : AT(scratch + (bss - data))
  {
    bss = .;
    *(.bss)
    . = ALIGN(4);
  } > scratchram
}


