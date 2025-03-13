// init: The initial user-level program

#include "kernel/include/types.h"
#include "kernel/include/stat.h"
#include "kernel/include/file.h"
#include "kernel/include/fcntl.h"
#include "xv6-user/user.h"

char *argv[] = {"sh", 0};

char *tests[] = {
    "brk", "close", "execve", "fstat", "getpid", "mkdir_", "mount", "openat", "uname", "waitpid",
    "chdir", "dup", "exit", "getcwd", "getppid", "mmap", "munmap", "pipe", "sleep", "times",
    "unlink", "write", "clone", "dup2", "fork", "getdents", "gettimeofday", "open", "read", "umount",
    "wait", "yield"};
int num = sizeof(tests) / sizeof((tests)[0]);

int main(void)
{
  int pid, wpid;

  // if(open("console", O_RDWR) < 0){
  //   mknod("console", CONSOLE, 0);
  //   open("console", O_RDWR);
  // }
  dev(O_RDWR, CONSOLE, 0);
  dup(0); // stdout
  dup(0); // stderr
  for (int i = 0; i < num; i++)
  {
    printf("\ninit: starting %d\n", i);
    pid = fork();
    if (pid < 0)
    {
      printf("init: fork failed\n");
      exit(1);
    }
    if (pid == 0)
    {
      exec(tests[i], argv);
      printf("init: exec %s failed\n", tests[i]);
      exit(1);
    }

    for (;;)
    {
      // this call to wait() returns if the shell exits,
      // or if a parentless process exits.
      wpid = wait((int *)0);
      if (wpid == pid)
      {
        // the shell exited; restart it.
        break;
      }
      else if (wpid < 0)
      {
        printf("init: wait returned an error\n");
        exit(1);
      }
      else
      {
        // it was a parentless process; do nothing.
      }
    }
  }

  shutdown();

  // 程序不会执行到这里，但为了语法完整性写一个 return 0
  return 0;
}