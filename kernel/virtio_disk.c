//
// driver for NEMU's synchronous MMIO disk device (sync_disk_t).
// replaces the VirtIO driver -- operations complete immediately,
// no interrupts needed.
//

#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "fs.h"
#include "buf.h"

// sync_disk_t register offsets (relative to VIRTIO0 = 0x10001000)
#define SYNC_DISK_CMD       0x00
#define SYNC_DISK_STATUS    0x04
#define SYNC_DISK_COUNT     0x08
#define SYNC_DISK_LBA_LOW   0x10
#define SYNC_DISK_LBA_HIGH  0x14
#define SYNC_DISK_PA_LOW    0x18
#define SYNC_DISK_PA_HIGH   0x1c
#define SYNC_DISK_ERROR     0x20

#define SYNC_DISK_CMD_NONE  0
#define SYNC_DISK_CMD_READ  1
#define SYNC_DISK_CMD_WRITE 2

#define SYNC_DISK_STATUS_IDLE  0
#define SYNC_DISK_STATUS_DONE  1
#define SYNC_DISK_STATUS_ERROR 2

#define R(r) ((volatile uint32 *)(VIRTIO0 + (r)))

static struct spinlock disk_lock;

void
virtio_disk_init(void)
{
  initlock(&disk_lock, "sync_disk");
}

void
virtio_disk_rw(struct buf *b, int write)
{
  uint64 sector = b->blockno * (BSIZE / 512);
  uint64 pa = (uint64)b->data;

  acquire(&disk_lock);

  *R(SYNC_DISK_COUNT)    = BSIZE / 512;
  *R(SYNC_DISK_LBA_LOW)  = (uint32)sector;
  *R(SYNC_DISK_LBA_HIGH) = (uint32)(sector >> 32);
  *R(SYNC_DISK_PA_LOW)   = (uint32)pa;
  *R(SYNC_DISK_PA_HIGH)  = (uint32)(pa >> 32);

  // Flush D-cache BEFORE the DMA: for a write, pushes dirty buffer bytes out
  // to SDRAM so the device sees them; for a read, evicts any dirty lines that
  // would otherwise clobber the DMA-transferred bytes when later written back.
  asm volatile("fence.i");

  *R(SYNC_DISK_CMD) = write ? SYNC_DISK_CMD_WRITE : SYNC_DISK_CMD_READ;

  if(*R(SYNC_DISK_STATUS) != SYNC_DISK_STATUS_DONE)
    panic("sync_disk: operation failed");

  // DMA bypasses the D-cache; force a cache flush so the CPU sees the freshly
  // transferred buffer data (for reads) and any pending dirty writeback hits
  // memory before handing the buffer back (for writes).
  asm volatile("fence.i");

  release(&disk_lock);
}

void
virtio_disk_intr(void)
{
}
