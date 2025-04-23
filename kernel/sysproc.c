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

/**
 * sys_sbrk - Adjusts the size of the process's data segment (heap).
 *
 * This system call is used to grow or shrink the heap of the calling process.
 * If n == 0, it returns the current size of the process's memory (heap end address).
 * Otherwise, it attempts to change the heap size to n bytes.
 *
 * Parameters:
 *   - n (int): The new size of the process's heap in bytes, fetched from the first syscall argument.
 *
 * Returns:
 *   - (uint64) If n == 0, returns the current heap end address (process size).
 *   - (uint64) If successful in resizing, returns 0.
 *   - (uint64) If an error occurs (invalid argument or growproc fails), returns -1.
 */
uint64
sys_sbrk(void)
{
  int addr;
  int n;

  // Fetch the first argument (desired heap size) from the user.
  if (argint(0, &n) < 0)
    return -1;

  // Get the current size of the process's address space.
  addr = myproc()->sz;

  // If n == 0, return the current heap end address.
  if (n == 0)
    return addr;

  // Attempt to grow or shrink the process's heap to the new size.
  if (growproc(n - addr) < 0)
    return -1;

  // Return 0 on success.
  return 0;
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

/**
 * @brief Wait for a specific child process to change state.
 *
 * This system call suspends execution of the calling process until the child process
 * specified by pid changes state. The status of the child process is stored at the
 * user-provided address p. The options parameter can be used to modify the behavior.
 *
 * @param pid (int): The process ID of the child to wait for, fetched from syscall argument 0.
 * @param p (uint64, user pointer): User-space address to store the exit status, fetched from syscall argument 1.
 * @param options (int): Options to modify wait behavior, fetched from syscall argument 2.
 * @return uint64: Returns the PID of the child process that changed state, or -1 on error.
 */
uint64 sys_waitpid(void)
{
  uint64 p;
  int pid, options;
  // Fetch syscall arguments: pid, status address, and options
  if (argint(0, &pid) < 0 || argaddr(1, &p) < 0 || argint(2, &options))
    return -1;

  // Call the kernel wait function with the provided arguments
  return wait(pid, p, options);
}

/**
 * @brief Create a new process (clone).
 *
 * This system call creates a new process, similar to fork, but may allow for more
 * fine-grained control over what is shared between parent and child.
 *
 * @return uint64: Returns the PID of the new process, or -1 on error.
 */
uint64 sys_clone(void)
{
  // Directly call the kernel clone function
  return clone();
}

/**
 * @brief Get the parent process ID of the calling process.
 *
 * This system call returns the process ID of the parent of the calling process.
 *
 * @return uint64: Returns the PID of the parent process.
 */
uint64 sys_getppid(void)
{
  // Return the PID of the parent process
  return myproc()->parent->pid;
}

/**
 * @brief Yield the processor voluntarily.
 *
 * This system call causes the calling process to yield the CPU, allowing the scheduler
 * to run another process.
 *
 * @return uint64: Always returns 0.
 */
uint64 sys_sched_yield(void)
{
  // Yield the CPU to another process
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
