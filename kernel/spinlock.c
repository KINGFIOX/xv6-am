// Mutual exclusion spin locks.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "proc.h"
#include "defs.h"

void
initlock(struct spinlock *lk, char *name)
{
  lk->name = name;
  lk->locked = 0;
  lk->cpu = 0;
}

// Acquire the lock.
// Loops (spins) until the lock is acquired.
void
acquire(struct spinlock *lk)
{
  push_off(); // disable interrupts to avoid deadlock.
  if(holding(lk))
    panic("acquire");

  // Single-core build: the NPC does not implement the A extension, so we
  // cannot use amoswap-based test-and-set.  With interrupts disabled and
  // only one hart, a "busy" lock can never be released from underneath us,
  // so encountering lk->locked == 1 means the current hart already holds
  // it (or something is seriously wrong).  Treat that as a deadlock panic
  // rather than spinning forever.
  if(lk->locked)
    panic("acquire: deadlock");
  lk->locked = 1;

  // Still emit a compiler+hardware memory barrier so the critical section's
  // memory references don't leak above the lock acquire.  "fence rw,rw"
  // works on in-order RV64 without needing the A extension.
  __asm__ volatile ("fence rw,rw" ::: "memory");

  // Record info about lock acquisition for holding() and debugging.
  lk->cpu = mycpu();
}

// Release the lock.
void
release(struct spinlock *lk)
{
  if(!holding(lk))
    panic("release");

  lk->cpu = 0;

  // Still emit a memory barrier to keep the critical section's stores from
  // sinking past the release.  Plain "fence rw,rw" needs no A extension.
  __asm__ volatile ("fence rw,rw" ::: "memory");

  // Single-core build: no A extension on the NPC, so we can't use the
  // amoswap-based __sync_lock_release.  A plain store of 0 is fine here
  // because interrupts are still disabled (pop_off runs after) and there
  // are no other harts.
  lk->locked = 0;

  pop_off();
}

// Check whether this cpu is holding the lock.
// Interrupts must be off.
int
holding(struct spinlock *lk)
{
  int r;
  r = (lk->locked && lk->cpu == mycpu());
  return r;
}

// push_off/pop_off are like intr_off()/intr_on() except that they are matched:
// it takes two pop_off()s to undo two push_off()s.  Also, if interrupts
// are initially off, then push_off, pop_off leaves them off.

void
push_off(void)
{
  int old = intr_get();

  // disable interrupts to prevent an involuntary context
  // switch while using mycpu().
  intr_off();

  if(mycpu()->noff == 0)
    mycpu()->intena = old;
  mycpu()->noff += 1;
}

void
pop_off(void)
{
  struct cpu *c = mycpu();
  if(intr_get())
    panic("pop_off - interruptible");
  if(c->noff < 1)
    panic("pop_off");
  c->noff -= 1;
  if(c->noff == 0 && c->intena)
    intr_on();
}
