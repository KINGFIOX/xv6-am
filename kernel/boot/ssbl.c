extern void _entry(void);

extern int _app_lma;
extern int _app_vma_start;
extern int _app_vma_end;
extern int _bss_start;
extern int _bss_end;

void ssbl(void) {
  // copy .text, .rodata, .data from flash(xip) to sdram
  volatile int *src = &_app_lma;
  volatile int *dst = &_app_vma_start;
  volatile int *end = &_app_vma_end;
  while (dst < end) {
    *dst++ = *src++;
  }

  // clear .bss
  dst = &_bss_start;
  end = &_bss_end;
  while (dst < end) {
    *dst++ = 0;
  }
  asm volatile("fence.i" ::: "memory");

  // Jump to xv6 kernel entry (stays in M-mode; xv6 start() handles the
  // M->S mode switch itself).
  _entry();
}
