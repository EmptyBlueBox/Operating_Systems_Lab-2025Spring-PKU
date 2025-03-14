#ifndef __SYSCALL_H
#define __SYSCALL_H

#include "types.h"
#include "sysnum.h"

struct utsname
{
    char sysname[65];
    char nodename[65];
    char release[65];
    char version[65];
    char machine[65];
    char domainname[65];
};

int fetchaddr(uint64 addr, uint64 *ip);
int fetchstr(uint64 addr, char *buf, int max);
int argint(int n, int *ip);
int argaddr(int n, uint64 *ip);
int argstr(int n, char *buf, int max);
void syscall(void);

#endif