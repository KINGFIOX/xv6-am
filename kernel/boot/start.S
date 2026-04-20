/* NPC 平台两级 bootloader 入口
 * 1. 初始化 s0 和 sp
 * 2. 调用 fsbl，把 ssbl 从 flash 拷贝到 sdram
 *
 * 与 abstract-machine/am/src/platform/npc/start.S 保持一致的思路，
 * 只是入口符号命名为 _bootstart 以避免和 xv6 已有的 _entry 冲突。
 */

.section entry, "ax"
.globl _bootstart
.type _bootstart, @function

_bootstart:
  mv    s0, zero
  la    sp, _stack_pointer

  call  fsbl

.size _bootstart, . - _bootstart
