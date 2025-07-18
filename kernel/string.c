#include "include/types.h"

void *
memset(void *dst, int c, uint n)
{
  char *cdst = (char *)dst;
  int i;
  for (i = 0; i < n; i++)
  {
    cdst[i] = c;
  }
  return dst;
}

int memcmp(const void *v1, const void *v2, uint n)
{
  const uchar *s1, *s2;

  s1 = v1;
  s2 = v2;
  while (n-- > 0)
  {
    if (*s1 != *s2)
      return *s1 - *s2;
    s1++, s2++;
  }

  return 0;
}

void *
memmove(void *dst, const void *src, uint n)
{
  const char *s;
  char *d;

  s = src;
  d = dst;
  if (s < d && s + n > d)
  {
    s += n;
    d += n;
    while (n-- > 0)
      *--d = *--s;
  }
  else
    while (n-- > 0)
      *d++ = *s++;

  return dst;
}

// memcpy exists to placate GCC.  Use memmove.
void *
memcpy(void *dst, const void *src, uint n)
{
  return memmove(dst, src, n);
}

int strncmp(const char *p, const char *q, uint n)
{
  while (n > 0 && *p && *p == *q)
    n--, p++, q++;
  if (n == 0)
    return 0;
  return (uchar)*p - (uchar)*q;
}

char *
strncpy(char *s, const char *t, int n)
{
  char *os;

  os = s;
  while (n-- > 0 && (*s++ = *t++) != 0)
    ;
  while (n-- > 0)
    *s++ = 0;
  return os;
}

// Like strncpy but guaranteed to NUL-terminate.
char *
safestrcpy(char *s, const char *t, int n)
{
  char *os;

  os = s;
  if (n <= 0)
    return os;
  while (--n > 0 && (*s++ = *t++) != 0)
    ;
  *s = 0;
  return os;
}

int strlen(const char *s)
{
  int n;

  for (n = 0; s[n]; n++)
    ;
  return n;
}

// convert uchar string into wide char string
void wnstr(wchar *dst, char const *src, int len)
{
  while (len-- && *src)
  {
    *(uchar *)dst = *src++;
    dst++;
  }

  *dst = 0;
}

// convert wide char string into uchar string
void snstr(char *dst, wchar const *src, int len)
{
  while (len-- && *src)
  {
    *dst++ = (uchar)(*src & 0xff);
    src++;
  }
  while (len-- > 0)
    *dst++ = 0;
}

int wcsncmp(wchar const *s1, wchar const *s2, int len)
{
  int ret = 0;

  while (len-- && *s1)
  {
    ret = (int)(*s1++ - *s2++);
    if (ret)
      break;
  }

  return ret;
}

char *
strchr(const char *s, char c)
{
  for (; *s; s++)
    if (*s == c)
      return (char *)s;
  return 0;
}

char *str_mycat(char *dest, const char *src, int max_len)
{
  int dest_len = strlen(dest);
  int remain_space = max_len - dest_len - 1; // 减1确保有空间放置字符串结尾的空字符
  if (remain_space > 0)
  {
    strncpy(dest + dest_len, src, remain_space);
  }
  dest[max_len - 1] = '\0'; // 确保字符串以空字符结尾
  return dest;
}
