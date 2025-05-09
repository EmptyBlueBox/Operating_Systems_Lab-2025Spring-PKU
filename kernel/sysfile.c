//
// File-system system calls.
// Mostly argument checking, since we don't trust
// user code, and calls into file.c and fs.c.
//

#include "include/types.h"
#include "include/riscv.h"
#include "include/param.h"
#include "include/stat.h"
#include "include/spinlock.h"
#include "include/proc.h"
#include "include/sleeplock.h"
#include "include/file.h"
#include "include/pipe.h"
#include "include/fcntl.h"
#include "include/fat32.h"
#include "include/syscall.h"
#include "include/string.h"
#include "include/printf.h"
#include "include/vm.h"

/**
 * 获取当前目录的绝对路径。
 *
 * @param cwd (struct dirent*): 当前目录项指针。
 * @param path (char*): 用于存储绝对路径的缓冲区。
 * @return int: 成功返回0，失败返回-1。
 */
int get_abspath(struct dirent *cwd, char *path)
{
  if (path == NULL)
    return -1;
  strncpy(path, cwd->filename, FAT32_MAX_FILENAME + 1);
  char temp[FAT32_MAX_FILENAME];
  while (cwd->parent != NULL)
  {
    cwd = cwd->parent;
    strncpy(temp, cwd->filename, FAT32_MAX_FILENAME);
    if (temp == NULL)
      return -1;
    str_mycat(temp, "/", FAT32_MAX_FILENAME);
    str_mycat(temp, path, FAT32_MAX_FILENAME);
    strncpy(path, temp, FAT32_MAX_FILENAME);
    if (path == NULL)
      return -1;
  }
  return 0;
}

/**
 * 根据文件描述符和路径名获取绝对路径。
 *
 * @param path (char*): 路径缓冲区。
 * @param fd (int): 文件描述符。
 * @return int: 成功返回0，失败返回-1。
 */
int get_path(char *path, int fd)
{
  if (path == NULL)
  {
    printf("path == null\n");
    return -1;
  }

  if (path[0] == '/')
  {
    return 0;
  }
  else if (fd == AT_FDCWD)
  {
    struct proc *current_proc = myproc();
    struct dirent *cwd = current_proc->cwd;
    char parent_name[FAT32_MAX_FILENAME + 1];
    if (get_abspath(cwd, parent_name) < 0)
    {
      printf("wrong path\n");
      return -1;
    }
    str_mycat(parent_name, "/", FAT32_MAX_FILENAME);
    str_mycat(parent_name, path, FAT32_MAX_FILENAME);
    strncpy(path, parent_name, FAT32_MAX_FILENAME);
    return 0;
  }
  else
  {
    if (fd < 0)
      return -1;

    struct proc *current_proc = myproc();
    struct file *f = current_proc->ofile[fd];
    if (f == 0)
      return -1;

    struct dirent *cwd = f->ep;
    char dirname[FAT32_MAX_FILENAME + 1];
    if (get_abspath(cwd, dirname) < 0)
    {
      printf("wrong path\n");
      return -1;
    }
    str_mycat(dirname, "/", FAT32_MAX_FILENAME);
    str_mycat(dirname, path, FAT32_MAX_FILENAME);
    strncpy(path, dirname, FAT32_MAX_FILENAME);
    return 0;
  }
}

/**
 * 获取第 n 个系统调用参数作为文件描述符，并返回对应的 struct file 指针。
 *
 * @param n (int): 参数索引。
 * @param pfd (int*): 用于返回文件描述符的指针。
 * @param pf (struct file**): 用于返回 struct file 指针的指针。
 * @return int: 成功返回0，失败返回-1。
 */
static int
argfd(int n, int *pfd, struct file **pf)
{
  int fd;
  struct file *f;

  if (argint(n, &fd) < 0)
    return -1;
  if (fd < 0 || fd >= NOFILE || (f = myproc()->ofile[fd]) == NULL)
    return -1;
  if (pfd)
    *pfd = fd;
  if (pf)
    *pf = f;
  return 0;
}

/**
 * 为给定的文件分配一个文件描述符。
 *
 * @param f (struct file*): 文件结构体指针。
 * @return int: 成功返回文件描述符，失败返回-1。
 */
static int
fdalloc(struct file *f)
{
  int fd;
  struct proc *p = myproc();

  for (fd = 0; fd < NOFILE; fd++)
  {
    if (p->ofile[fd] == 0)
    {
      p->ofile[fd] = f;
      return fd;
    }
  }
  return -1;
}

/**
 * 复制一个文件描述符。
 *
 * @return uint64: 新的文件描述符，失败返回-1。
 */
uint64
sys_dup(void)
{
  struct file *f;
  int fd;

  if (argfd(0, 0, &f) < 0)
    return -1;
  if ((fd = fdalloc(f)) < 0)
    return -1;
  filedup(f);
  return fd;
}

/**
 * 从文件中读取数据。
 *
 * @return uint64: 实际读取的字节数，失败返回-1。
 */
uint64
sys_read(void)
{
  struct file *f;
  int n;
  uint64 p;

  if (argfd(0, 0, &f) < 0 || argint(2, &n) < 0 || argaddr(1, &p) < 0)
    return -1;
  return fileread(f, p, n);
}

/**
 * 向文件写入数据。
 *
 * @return uint64: 实际写入的字节数，失败返回-1。
 */
uint64
sys_write(void)
{
  struct file *f;
  int n;
  uint64 p;

  if (argfd(0, 0, &f) < 0 || argint(2, &n) < 0 || argaddr(1, &p) < 0)
    return -1;

  return filewrite(f, p, n);
}

/**
 * 关闭一个文件描述符。
 *
 * @return uint64: 成功返回0，失败返回-1。
 */
uint64
sys_close(void)
{
  int fd;
  struct file *f;

  if (argfd(0, &fd, &f) < 0)
  {
    printf("close fd: %d\n", argfd(0, &fd, &f));
    return -1;
  }
  myproc()->ofile[fd] = 0;
  fileclose(f);
  return 0;
}

struct kstat
{
  uint64 st_dev;
  uint64 st_ino;
  unsigned int st_mode;
  uint32 st_nlink;
  uint32 st_uid;
  uint32 st_gid;
  uint64 st_rdev;
  unsigned long __pad;
  long int st_size;
  uint32 st_blksize;
  int __pad2;
  uint64 st_blocks;
  long st_atime_sec;
  long st_atime_nsec;
  long st_mtime_sec;
  long st_mtime_nsec;
  long st_ctime_sec;
  long st_ctime_nsec;
  unsigned __unused[2];
};

/**
 * 获取文件状态信息。
 *
 * @return uint64: 成功返回0，失败返回-1。
 */
uint64 sys_fstat(void)
{
  int fd;
  uint64 addr;
  if (argint(0, &fd) < 0 || argaddr(1, &addr) < 0)
    return -1;
  struct proc *p = myproc();
  struct file *f = p->ofile[fd];
  struct dirent *ep = f->ep;
  struct kstat *st = {0};
  st->st_dev = ep->dev;
  st->st_ino = 0;
  st->st_mode = (ep->attribute & ATTR_DIRECTORY) ? T_DIR : T_FILE;
  st->st_nlink = f->ref;
  st->st_size = ep->file_size;
  // st->st_atime_sec = ep->atime / 10000000;
  // st->st_mtime_sec = ep->mtime / 10000000;
  // st->st_ctime_sec = ep->ctime / 10000000;
  *(struct kstat *)addr = *st;
  return 0;
}

/**
 * 创建一个新的目录项（文件或目录）。
 *
 * @param path (char*): 路径。
 * @param type (short): 类型（T_FILE 或 T_DIR）。
 * @param mode (int): 模式。
 * @return struct dirent*: 成功返回新目录项指针，失败返回NULL。
 */
static struct dirent *
create(char *path, short type, int mode)
{
  struct dirent *ep, *dp;
  char name[FAT32_MAX_FILENAME + 1];

  if ((dp = enameparent(path, name)) == NULL)
    return NULL;

  if (type == T_DIR)
  {
    mode = ATTR_DIRECTORY;
  }
  else if (mode & O_RDONLY)
  {
    mode = ATTR_READ_ONLY;
  }
  else
  {
    mode = 0;
  }

  elock(dp);
  if ((ep = ealloc(dp, name, mode)) == NULL)
  {
    eunlock(dp);
    eput(dp);
    return NULL;
  }

  if ((type == T_DIR && !(ep->attribute & ATTR_DIRECTORY)) ||
      (type == T_FILE && (ep->attribute & ATTR_DIRECTORY)))
  {
    eunlock(dp);
    eput(ep);
    eput(dp);
    return NULL;
  }

  eunlock(dp);
  eput(dp);

  elock(ep);
  return ep;
}

/**
 * 打开一个文件。
 *
 * @return uint64: 成功返回文件描述符，失败返回-1。
 */
uint64
sys_open(void)
{
  char path[FAT32_MAX_PATH];
  int fd, omode;
  struct file *f;
  struct dirent *ep;

  if (argstr(0, path, FAT32_MAX_PATH) < 0 || argint(1, &omode) < 0)
    return -1;

  if (omode & O_CREATE)
  {
    ep = create(path, T_FILE, omode);
    if (ep == NULL)
    {
      return -1;
    }
  }
  else
  {
    if ((ep = ename(path)) == NULL)
    {
      return -1;
    }
    elock(ep);
    if ((ep->attribute & ATTR_DIRECTORY) && omode != O_RDONLY)
    {
      eunlock(ep);
      eput(ep);
      return -1;
    }
  }

  if ((f = filealloc()) == NULL || (fd = fdalloc(f)) < 0)
  {
    if (f)
    {
      fileclose(f);
    }
    eunlock(ep);
    eput(ep);
    return -1;
  }

  if (!(ep->attribute & ATTR_DIRECTORY) && (omode & O_TRUNC))
  {
    etrunc(ep);
  }

  f->type = FD_ENTRY;
  f->off = (omode & O_APPEND) ? ep->file_size : 0;
  f->ep = ep;
  f->readable = !(omode & O_WRONLY);
  f->writable = (omode & O_WRONLY) || (omode & O_RDWR);

  eunlock(ep);

  return fd;
}

/**
 * 以指定目录为基础打开文件。
 *
 * @return uint64: 成功返回文件描述符，失败返回-1。
 */
uint64 sys_openat(void)
{
  int fd;
  char path[FAT32_MAX_PATH];
  int flags;
  int mode;
  struct dirent *ep;
  struct file *f;
  if (argint(0, &fd) < 0 || argstr(1, path, FAT32_MAX_PATH) < 0 || argint(2, &flags) < 0 || argint(3, &mode) < 0)
    return -1;
  if (*path == '\0')
    return -1;

  if (get_path(path, fd) < 0)
  {
    printf("error in openat\n");
    return -1;
  }

  int new_fd;
  if (flags & O_CREATE)
  {
    ep = create(path, T_FILE, flags);
    if (ep == NULL)
    {
      printf("creat null: %d\n", flags);
      return -1;
    }
  }
  else
  {
    if ((ep = ename(path)) == NULL)
    {
      return -1;
    }
    elock(ep);
    if ((ep->attribute & ATTR_DIRECTORY) && (flags & O_WRONLY))
    {
      eunlock(ep);
      eput(ep);
      printf("show O_DIRECTORY: %d \n", flags);
      printf("abs_path=%s\n", path);
      return -1;
    }
  }

  if ((f = filealloc()) == NULL || (new_fd = fdalloc(f)) < 0)
  {
    if (f)
    {
      fileclose(f);
    }
    eunlock(ep);
    eput(ep);
    printf("unable to open: %d\n", flags);
    return -1;
  }

  if (!(ep->attribute & ATTR_DIRECTORY) && (flags & O_TRUNC))
  {
    etrunc(ep);
  }

  f->type = FD_ENTRY;
  f->off = (flags & O_APPEND) ? ep->file_size : 0;
  f->ep = ep;
  f->readable = !(flags & O_WRONLY);
  f->writable = (flags & O_WRONLY) || (flags & O_RDWR);

  eunlock(ep);

  return new_fd;
}

/**
 * 创建一个目录。
 *
 * @return uint64: 成功返回0，失败返回-1。
 */
uint64
sys_mkdir(void)
{
  char path[FAT32_MAX_PATH];
  struct dirent *ep;

  if (argstr(0, path, FAT32_MAX_PATH) < 0 || (ep = create(path, T_DIR, 0)) == 0)
  {
    return -1;
  }
  eunlock(ep);
  eput(ep);
  return 0;
}

/**
 * 以指定目录为基础创建目录。
 *
 * @return uint64: 成功返回0，失败返回-1。
 */
uint64
sys_mkdirat(void)
{
  int dirfd;
  char path[FAT32_MAX_PATH];
  int mode;
  if (argint(0, &dirfd) < 0 || argstr(1, path, FAT32_MAX_PATH) < 0 || argint(2, &mode) < 0)
  {
    printf("wrong input\n");
    return -1;
  }

  if (*path == '\0')
    return -1;

  if (get_path(path, dirfd) < 0)
  {
    printf("error in mkdirat\n");
    return -1;
  }

  struct dirent *ep;
  ep = create(path, T_DIR, 0);
  eunlock(ep);
  eput(ep);
  return 0;
}

/**
 * 切换当前工作目录。
 *
 * @return uint64: 成功返回0，失败返回-1。
 */
uint64
sys_chdir(void)
{
  char path[FAT32_MAX_PATH];
  struct dirent *ep;
  struct proc *p = myproc();

  if (argstr(0, path, FAT32_MAX_PATH) < 0 || (ep = ename(path)) == NULL)
  {
    return -1;
  }
  elock(ep);
  if (!(ep->attribute & ATTR_DIRECTORY))
  {
    eunlock(ep);
    eput(ep);
    return -1;
  }
  eunlock(ep);
  eput(p->cwd);
  p->cwd = ep;
  return 0;
}

/**
 * 创建管道并返回两个文件描述符。
 *
 * @return uint64: 成功返回0，失败返回-1。
 */
uint64
sys_pipe(void)
{
  uint64 fdarray;            // 用户空间的指针，指向两个整数的数组
  struct file *rf, *wf;      // rf: 读端文件结构指针, wf: 写端文件结构指针
  int fd0, fd1;              // fd0: 读端文件描述符, fd1: 写端文件描述符
  struct proc *p = myproc(); // 当前进程指针

  // 获取用户传入的 fdarray 参数
  if (argaddr(0, &fdarray) < 0)
    return -1;
  // 分配管道的读写 file 结构
  if (pipealloc(&rf, &wf) < 0)
    return -1;
  fd0 = -1;
  // 分配读端和写端的文件描述符
  if ((fd0 = fdalloc(rf)) < 0 || (fd1 = fdalloc(wf)) < 0)
  {
    // 如果只分配了读端描述符，回收
    if (fd0 >= 0)
      p->ofile[fd0] = 0;
    fileclose(rf);
    fileclose(wf);
    return -1;
  }
  // 将文件描述符写回用户空间
  if (copyout2(fdarray, (char *)&fd0, sizeof(fd0)) < 0 ||
      copyout2(fdarray + sizeof(fd0), (char *)&fd1, sizeof(fd1)) < 0)
  {
    // 回收已分配的文件描述符和 file 结构
    p->ofile[fd0] = 0;
    p->ofile[fd1] = 0;
    fileclose(rf);
    fileclose(wf);
    return -1;
  }
  return 0;
}

/**
 * 打开控制台设备。
 *
 * @return uint64: 成功返回文件描述符，失败返回-1。
 */
uint64
sys_dev(void)
{
  int fd, omode;
  int major, minor;
  struct file *f;

  if (argint(0, &omode) < 0 || argint(1, &major) < 0 || argint(2, &minor) < 0)
  {
    return -1;
  }

  if (omode & O_CREATE)
  {
    panic("dev file on FAT");
  }

  if (major < 0 || major >= NDEV)
    return -1;

  if ((f = filealloc()) == NULL || (fd = fdalloc(f)) < 0)
  {
    if (f)
      fileclose(f);
    return -1;
  }

  f->type = FD_DEVICE;
  f->off = 0;
  f->ep = 0;
  f->major = major;
  f->readable = !(omode & O_WRONLY);
  f->writable = (omode & O_WRONLY) || (omode & O_RDWR);

  return fd;
}

/**
 * 读取目录项（支持 ls 命令）。
 *
 * @return uint64: 成功返回0，失败返回-1。
 */
uint64
sys_readdir(void)
{
  struct file *f;
  uint64 p;

  if (argfd(0, 0, &f) < 0 || argaddr(1, &p) < 0)
    return -1;
  return dirnext(f, p);
}

/**
 * 获取当前工作目录的绝对路径。
 *
 * @return uint64: 成功返回路径字符串的用户空间地址，失败返回0。
 */
uint64
sys_getcwd(void)
{
  struct proc *p;
  uint64 addr;
  int size;

  // Fetch syscall arguments: buffer address and buffer size
  if (argaddr(0, &addr) < 0 || argint(1, &size))
    return 0;

  struct dirent *de = myproc()->cwd;
  char path[FAT32_MAX_PATH];
  char *s;
  int len;

  // If current directory is root, path is "/"
  if (de->parent == NULL)
  {
    s = "/";
  }
  else
  {
    // Fill the path buffer from the end backwards
    s = path + FAT32_MAX_PATH - 1;
    *s = '\0';
    while (de->parent)
    {
      len = strlen(de->filename);
      s -= len;
      if (s <= path) // Not enough space to reach root
        return 0;
      strncpy(s, de->filename, len);
      *--s = '/';
      de = de->parent;
    }
  }

  // If addr is 0, allocate buffer on user stack
  if (addr == 0)
  {
    p = myproc();
    // Check if there is enough space on the stack
    if (p->trapframe->sp < (strlen(s) + 1))
      return NULL;
    p->trapframe->sp -= strlen(s) + 1;
    addr = p->trapframe->sp;
  }
  else
  {
    int path_length = strlen(s) + 1;
    // Check if user buffer is large enough
    if (size < path_length)
      return NULL;
  }

  // Copy the path string to user space
  if (copyout2(addr, s, strlen(s) + 1) < 0)
    return NULL;

  return addr;
}

/**
 * 判断目录是否为空（只包含 . 和 ..）。
 *
 * @param dp (struct dirent*): 目录项指针。
 * @return int: 空返回1，否则返回0。
 */
static int
isdirempty(struct dirent *dp)
{
  struct dirent ep;
  int count;
  int ret;
  ep.valid = 0;
  ret = enext(dp, &ep, 2 * 32, &count); // skip the "." and ".."
  return ret == -1;
}

/**
 * 删除文件或目录。
 *
 * @return uint64: 成功返回0，失败返回-1。
 */
uint64
sys_remove(void)
{
  char path[FAT32_MAX_PATH];
  struct dirent *ep;
  int len;
  if ((len = argstr(0, path, FAT32_MAX_PATH)) <= 0)
    return -1;

  char *s = path + len - 1;
  while (s >= path && *s == '/')
  {
    s--;
  }
  if (s >= path && *s == '.' && (s == path || *--s == '/'))
  {
    return -1;
  }

  if ((ep = ename(path)) == NULL)
  {
    return -1;
  }
  elock(ep);
  if ((ep->attribute & ATTR_DIRECTORY) && !isdirempty(ep))
  {
    eunlock(ep);
    eput(ep);
    return -1;
  }
  elock(ep->parent); // Will this lead to deadlock?
  eremove(ep);
  eunlock(ep->parent);
  eunlock(ep);
  eput(ep);

  return 0;
}

/**
 * 重命名文件或目录。
 *
 * @return uint64: 成功返回0，失败返回-1。
 */
uint64
sys_rename(void)
{
  char old[FAT32_MAX_PATH], new[FAT32_MAX_PATH];
  if (argstr(0, old, FAT32_MAX_PATH) < 0 || argstr(1, new, FAT32_MAX_PATH) < 0)
  {
    return -1;
  }

  struct dirent *src = NULL, *dst = NULL, *pdst = NULL;
  int srclock = 0;
  char *name;
  if ((src = ename(old)) == NULL || (pdst = enameparent(new, old)) == NULL || (name = formatname(old)) == NULL)
  {
    goto fail; // src doesn't exist || dst parent doesn't exist || illegal new name
  }
  for (struct dirent *ep = pdst; ep != NULL; ep = ep->parent)
  {
    if (ep == src)
    { // In what universe can we move a directory into its child?
      goto fail;
    }
  }

  uint off;
  elock(src); // must hold child's lock before acquiring parent's, because we do so in other similar cases
  srclock = 1;
  elock(pdst);
  dst = dirlookup(pdst, name, &off);
  if (dst != NULL)
  {
    eunlock(pdst);
    if (src == dst)
    {
      goto fail;
    }
    else if (src->attribute & dst->attribute & ATTR_DIRECTORY)
    {
      elock(dst);
      if (!isdirempty(dst))
      { // it's ok to overwrite an empty dir
        eunlock(dst);
        goto fail;
      }
      elock(pdst);
    }
    else
    { // src is not a dir || dst exists and is not an dir
      goto fail;
    }
  }

  if (dst)
  {
    eremove(dst);
    eunlock(dst);
  }
  memmove(src->filename, name, FAT32_MAX_FILENAME);
  emake(pdst, src, off);
  if (src->parent != pdst)
  {
    eunlock(pdst);
    elock(src->parent);
  }
  eremove(src);
  eunlock(src->parent);
  struct dirent *psrc = src->parent; // src must not be root, or it won't pass the for-loop test
  src->parent = edup(pdst);
  src->off = off;
  src->valid = 1;
  eunlock(src);

  eput(psrc);
  if (dst)
  {
    eput(dst);
  }
  eput(pdst);
  eput(src);

  return 0;

fail:
  if (srclock)
    eunlock(src);
  if (dst)
    eput(dst);
  if (pdst)
    eput(pdst);
  if (src)
    eput(src);
  return -1;
}

/**
 * 复制文件描述符到指定的新描述符。
 *
 * @return uint64: 成功返回新文件描述符，失败返回-1。
 */
uint64
sys_dup3(void)
{
  // printf("dup3\n");
  struct file *f;
  int old_fd, new_fd;

  // 获取第一个参数 old_fd，并将其转换为文件结构指针 f
  if (argfd(0, &old_fd, &f) < 0)
    return -1;
  // 获取第二个参数 new_fd
  if (argint(1, &new_fd) < 0)
    return -1;

  // printf("old_fd: %d, new_fd: %d\n", old_fd, new_fd);
  // printf("NOFILE: %d\n", NOFILE);
  // 检查 new_fd 是否在合法范围内
  if (new_fd < 0 || new_fd > NOFILE)
    return -1;

  // 如果 new_fd 已经打开，先关闭它
  if (myproc()->ofile[new_fd])
  {
    fileclose(myproc()->ofile[new_fd]);
  }

  // 将文件结构指针 f 赋值给 new_fd，并增加引用计数
  myproc()->ofile[new_fd] = f;
  filedup(f);
  return new_fd;
}

/**
 * 解除内存映射。
 *
 * @return uint64: 成功返回0，失败返回-1。
 */
uint64 sys_munmap(void)
{
  uint64 addr;
  int len;

  if (argaddr(0, &addr) < 0 || argint(1, &len) < 0)
  {
    return -1;
  }

  struct proc *p = myproc();
  vmunmap(p->pagetable, addr, (len / PGSIZE), 0);
  return 0;
}

/**
 * 内存映射文件。
 *
 * @return uint64: 成功返回映射的地址，失败返回-1。
 */
uint64 sys_mmap(void)
{
  uint64 addr;
  int len, prot, flags, fd, off;

  if (argaddr(0, &addr) < 0 || argint(1, &len) < 0 || argint(2, &prot) < 0 || argint(3, &flags) < 0 || argint(4, &fd) < 0 || argint(5, &off))
    return -1;

  struct proc *p = myproc();
  struct file *f = p->ofile[fd];
  int n = len;

  // Check if the file descriptor is valid and refers to a mappable file type
  if (f == NULL || f->type != FD_ENTRY || f->ep == NULL)
  {
    return -1; // Invalid file descriptor or not a directory entry
  }

  if (addr == 0)
  {
    addr = p->sz;
    p->sz = uvmalloc(p->pagetable, p->kpagetable, p->sz, p->sz + n);
  }
  elock(f->ep);
  if (n > f->ep->file_size - off)
    n = f->ep->file_size - off;
  if ((n = eread(f->ep, 1, addr, off, n)) < 0)
  {
    eunlock(f->ep);
    return -1;
  }
  eunlock(f->ep);
  return addr;
}

// 兼容 Linux 的目录项结构体
struct linux_dirent64
{
  uint64 d_ino;            // inode 号
  uint64 d_off;            // 下一个目录项的偏移
  unsigned short d_reclen; // 目录项长度
  unsigned char d_type;    // 文件类型
  char d_name[];           // 文件名
};

/**
 * 获取目录项信息，类似于 Linux 的 getdents64。
 *
 * @return uint64: 实际读取的字节数，失败返回-1。
 */
uint64 sys_getdents(void)
{
  int fd, len;
  uint64 buf;

  // 获取参数，若有错误则返回-1
  if (argint(0, &fd) < 0 || argaddr(1, &buf) < 0 || argint(2, &len) < 0)
    return -1;

  struct proc *p = myproc();
  struct file *f = p->ofile[fd];
  struct dirent *ep = f->ep;

  // 调用 getdents64 获取目录项
  return getdents64(ep, buf, len);
}

/**
 * 删除文件或目录（实现 unlinkat 功能）。
 *
 * @return uint64: 成功返回0，失败返回-1。
 */
uint64 sys_unlink(void)
{
  int dirfd, flags;
  char path[FAT32_MAX_PATH];

  // 获取参数，若有错误则返回-1
  if (argint(0, &dirfd) < 0 || argstr(1, path, FAT32_MAX_PATH) < 0 || argint(2, &flags) < 0)
  {
    printf("error in unlinkat\n");
    return -1;
  }

  // 解析路径
  if (get_path(path, dirfd) < 0)
  {
    printf("wrong path\n");
    return -1;
  }

  struct dirent *ep;
  // 查找目录项
  if ((ep = ename(path)) == NULL)
  {
    return -1;
  }
  elock(ep);
  // 如果是目录，且不为空或标志位不正确，则不能删除
  if ((ep->attribute & ATTR_DIRECTORY) && ((!isdirempty(ep) && (flags & AT_REMOVEDIR) != 0) || (flags & AT_REMOVEDIR) == 0))
  {
    eunlock(ep);
    eput(ep);
    return -1;
  }
  // 加锁父目录，防止并发删除
  elock(ep->parent); // 这里可能会导致死锁
  eremove(ep);       // 删除目录项
  eunlock(ep->parent);
  eunlock(ep);
  eput(ep);

  return 0;
}

/**
 * 挂载文件系统。
 *
 * @return uint64: 成功返回0，失败返回-1。
 */
uint64 sys_mount(void)
{
  char special[FAT32_MAX_PATH], dir[FAT32_MAX_PATH], fstype[FAT32_MAX_PATH];
  uint64 flags, data;
  struct dirent *di;

  // 获取参数，若有错误则返回-1
  if (argstr(0, special, FAT32_MAX_PATH) < 0 || argstr(1, dir, FAT32_MAX_PATH) < 0 || argstr(2, fstype, FAT32_MAX_PATH) < 0 || argaddr(3, &flags) < 0 || argaddr(4, &data))
    return -1;

  // 只支持 vfat 文件系统
  if (strncmp((char *)fstype, "vfat", 5))
  {
    printf("wrong file type\n");
    return -1;
  }

  // 查找挂载点目录
  if ((di = ename(dir)) == NULL)
  {
    return -1;
  }

  // 实际挂载操作未实现
  return 0;
}

/**
 * 卸载文件系统。
 *
 * @return uint64: 成功返回0，失败返回-1。
 */
uint64 sys_umount(void)
{
  char special[FAT32_MAX_PATH];
  uint64 flags;
  struct dirent *sp;

  // 获取参数，若有错误则返回-1
  if (argstr(0, special, FAT32_MAX_PATH) < 0 || argaddr(1, &flags) < 0)
    return -1;

  // 查找设备目录项
  if ((sp = ename(special)) == NULL)
  {
    return -1;
  }

  // 实际卸载操作未实现
  return 0;
}
