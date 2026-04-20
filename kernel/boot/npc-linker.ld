OUTPUT_ARCH( "riscv" )
ENTRY(_bootstart)

MEMORY {
  irom (rx)  : ORIGIN = _flash_base, LENGTH = _flash_size
  dram (rwx) : ORIGIN = _sdram_base, LENGTH = _sdram_size
}

SECTIONS {
  /* ===== FSBL: execute in place from flash ===== */
  .text.fsbl : {
    *(entry)
    kernel/boot/start.o(.text*)
    kernel/boot/fsbl.o(.text*)
  } > irom

  .rodata.fsbl : {
    kernel/boot/fsbl.o(.rodata*)
    kernel/boot/fsbl.o(.srodata*)
  } > irom

  /* ===== SSBL: stored in flash, FSBL copies to SDRAM ===== */
  .text.ssbl : {
    _ssbl_vma_start = .;
    kernel/boot/ssbl.o(.text*)
    kernel/boot/ssbl.o(.rodata*)
    kernel/boot/ssbl.o(.srodata*)
    _ssbl_vma_end = .;
    . = ALIGN(64);
  } > dram AT> irom
  _ssbl_lma = LOADADDR(.text.ssbl);

  /* ===== xv6 kernel ("app"): stored in flash, SSBL copies to SDRAM =====
   * Everything that xv6 expects to land at 0x80000000 goes here.
   * We preserve xv6's original ordering from kernel/kernel.ld:
   *   - kernel/entry.o(_entry) must be first so _entry lives at 0x80000000
   *   - .text.* follow
   *   - trampoline page aligned to 0x1000, size must be exactly one page
   */
  .app : {
    _app_vma_start = .;

    kernel/entry.o(_entry)
    *(.text .text.*)

    . = ALIGN(0x1000);
    _trampoline = .;
    *(trampsec)
    . = ALIGN(0x1000);
    ASSERT(. - _trampoline == 0x1000, "error: trampoline larger than one page");
    PROVIDE(etext = .);

    . = ALIGN(16);
    *(.srodata .srodata.*)
    . = ALIGN(16);
    *(.rodata .rodata.*)

    . = ALIGN(16);
    *(.sdata .sdata.*)
    . = ALIGN(16);
    *(.data .data.*)

    _app_vma_end = .;
  } > dram AT> irom
  _app_lma = LOADADDR(.app);

  .bss : {
    _bss_start = .;
    . = ALIGN(16);
    *(.sbss .sbss.*)
    . = ALIGN(16);
    *(.bss .bss.*)
    _bss_end = .;
  } > dram

  PROVIDE(end = .);

  _stack_top = ALIGN(0x1000);
  . = _stack_top + 0x80000;
  _stack_pointer = .;
  _heap_start = ALIGN(0x1000);
}
