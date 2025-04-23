#include "include/types.h"
#include "include/riscv.h"
#include "include/param.h"
#include "include/memlayout.h"
#include "include/spinlock.h"
#include "include/proc.h"
#include "include/syscall.h"
#include "include/timer.h"
#include "include/kalloc.h"
#include "include/string.h"
#include "include/printf.h"
#include "include/vm.h"

extern int exec(char *path, char **argv);

uint64
sys_exec(void)
{
  char path[FAT32_MAX_PATH], *argv[MAXARG];
  int i;
  uint64 uargv, uarg;

  if (argstr(0, path, FAT32_MAX_PATH) < 0 || argaddr(1, &uargv) < 0)
  {
    return -1;
  }
  memset(argv, 0, sizeof(argv));
  for (i = 0;; i++)
  {
    if (i >= NELEM(argv))
    {
      goto bad;
    }
    if (fetchaddr(uargv + sizeof(uint64) * i, (uint64 *)&uarg) < 0)
    {
      goto bad;
    }
    if (uarg == 0)
    {
      argv[i] = 0;
      break;
    }
    argv[i] = kalloc();
    if (argv[i] == 0)
      goto bad;
    if (fetchstr(uarg, argv[i], PGSIZE) < 0)
      goto bad;
  }

  int ret = exec(path, argv);

  for (i = 0; i < NELEM(argv) && argv[i] != 0; i++)
    kfree(argv[i]);

  return ret;

bad:
  for (i = 0; i < NELEM(argv) && argv[i] != 0; i++)
    kfree(argv[i]);
  return -1;
}

uint64
sys_exit(void)
{
  int n;
  if (argint(0, &n) < 0)
    return -1;
  exit(n);
  return 0; // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return fork();
}

uint64
sys_wait(void)
{
  uint64 p;
  if (argaddr(0, &p) < 0)
    return -1;
  return wait(-1, p, 0);
}

uint64
sys_sbrk(void)
{
  int addr;
  int n;

  if (argint(0, &n) < 0)
    return -1;
  addr = myproc()->sz;
  if (n == 0) // n = 0，用于获取进程所占物理内存的大小
    return addr;
  if (growproc(n) < 0)
    return -1;
  return addr;
}

uint64
sys_sleep(void)
{
  int n;
  uint ticks0;

  if (argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while (ticks - ticks0 < n)
  {
    if (myproc()->killed)
    {
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

uint64
sys_kill(void)
{
  int pid;

  if (argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

uint64
sys_trace(void)
{
  int mask;
  if (argint(0, &mask) < 0)
  {
    return -1;
  }
  myproc()->tmask = mask;
  return 0;
}

uint64 sys_times(void)
{
  uint64 addr;
  if (argaddr(0, &addr) < 0)
    return -1;
  struct proc *p = myproc();
  uint64 tick = p->n_tick;
  *(uint64 *)addr = tick * 50;
  *((uint64 *)addr + 1) = tick * 50;
  *((uint64 *)addr + 2) = 0;
  *((uint64 *)addr + 3) = 0;
  return tick;
}

uint64 sys_waitpid(void)
{
  uint64 p;
  int pid, options;
  if (argint(0, &pid) < 0 || argaddr(1, &p) < 0 || argint(2, &options))
    return -1;

  return wait(pid, p, options);
}

uint64 sys_clone(void)
{
  return clone();
}

uint64 sys_getppid(void)
{
  return myproc()->parent->pid;
}

uint64 sys_sched_yield(void)
{
  yield();
  return 0;
}

/**
 * @brief Get the current time of day.
 *
 * This system call retrieves the current time from the hardware timer and writes the result
 * to the user-provided address. The time is split into seconds and microseconds since boot.
 *
 * @return uint64: Returns 0 on success, -1 on failure.
 *
 * @param (user pointer) arg0 (uint64*, addr): Address in user space where the result will be stored.
 *        The result is written as two consecutive uint64 values:
 *          - addr[0]: seconds (uint64)
 *          - addr[1]: microseconds (uint64)
 */
uint64 sys_gettimeofday(void)
{
  uint64 time;
  uint64 addr;
  // Get the user address where the result should be stored
  if (argaddr(0, &addr) < 0)
    return -1;
  // Read the current hardware time (in 100ns units)
  if ((time = r_time()) < 0)
    return -1;
  // Convert hardware time to seconds and microseconds
  uint64 sec = time / 10000000;        // 1 second = 10,000,000 * 100ns
  uint64 usec = (time / 10) % 1000000; // 1 microsecond = 10 * 100ns
  // Store the result in user memory: [seconds, microseconds]
  *(uint64 *)addr = sec;
  *((uint64 *)addr + 1) = usec;
  return 0;
}

/**
 * @brief Sleep for a specified time interval (in seconds and microseconds).
 *
 * This system call suspends the calling process for at least the specified time.
 * The time interval is provided as a struct (two uint64 values) in user space.
 *
 * @return uint64: Returns 0 on success, -1 if interrupted or on error.
 *
 * @param (user pointer) arg0 (uint64*, addr): Address in user space containing the sleep interval.
 *        The interval is specified as two consecutive uint64 values:
 *          - addr[0]: seconds (uint64)
 *          - addr[1]: microseconds (uint64)
 */
uint64 sys_nanosleep(void)
{
  uint64 addr;
  // Get the user address where the sleep interval is stored
  if (argaddr(0, &addr) < 0)
    return -1;
  // Read the sleep interval from user memory
  uint64 sec = *(uint64 *)addr;
  uint64 usec = *((uint64 *)addr + 1);
  // Convert the interval to kernel ticks (each tick = 50ms, so 20 ticks = 1s)
  uint64 n = (sec * 20 + usec / 50000000);
  uint64 ticks0;
  acquire(&tickslock);
  ticks0 = ticks;
  // Sleep until the required number of ticks has passed
  while (ticks - ticks0 < n)
  {
    // If the process is killed, abort the sleep
    if (myproc()->killed)
    {
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}
