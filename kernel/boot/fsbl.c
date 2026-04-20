extern void ssbl(void);

extern int _ssbl_lma;
extern int _ssbl_vma_start;
extern int _ssbl_vma_end;

void fsbl(void) {
  // copy .text, .rodata from flash(xip) to sdram
  volatile int *src = &_ssbl_lma;
  volatile int *dst = &_ssbl_vma_start;
  volatile int *end = &_ssbl_vma_end;
  while (dst < end) {
    *dst++ = *src++;
  }

  asm volatile("fence.i" ::: "memory");
  ssbl();
}
