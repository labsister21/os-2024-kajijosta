#include "../header/stdlib/string.h"
#include <stddef.h>
#include <stdint.h>

void *memset(void *s, int c, size_t n)
{
  uint8_t *buf = (uint8_t *)s;
  for (size_t i = 0; i < n; i++)
    buf[i] = (uint8_t)c;
  return s;
}

void *memcpy(void *restrict dest, const void *restrict src, size_t n)
{
  uint8_t *dstbuf = (uint8_t *)dest;
  const uint8_t *srcbuf = (const uint8_t *)src;
  for (size_t i = 0; i < n; i++)
    dstbuf[i] = srcbuf[i];
  return dstbuf;
}

int memcmp(const void *s1, const void *s2, size_t n)
{
  const uint8_t *buf1 = (const uint8_t *)s1;
  const uint8_t *buf2 = (const uint8_t *)s2;
  for (size_t i = 0; i < n; i++)
  {
    if (buf1[i] < buf2[i])
      return -1;
    else if (buf1[i] > buf2[i])
      return 1;
  }

  return 0;
}

void *memmove(void *dest, const void *src, size_t n)
{
  uint8_t *dstbuf = (uint8_t *)dest;
  const uint8_t *srcbuf = (const uint8_t *)src;
  if (dstbuf < srcbuf)
  {
    for (size_t i = 0; i < n; i++)
      dstbuf[i] = srcbuf[i];
  }
  else
  {
    for (size_t i = n; i != 0; i--)
      dstbuf[i - 1] = srcbuf[i - 1];
  }

  return dest;
}

int strcmp(const char *s1, const char *s2)
{
  while (*s1 && *s1 == *s2)
  {
    s1++;
    s2++;
  }

  if (*s1 == '\0' && *s2 == '\0')
  {
    return 1;
  }
  else
  {
    return 0;
  }
}

// int strlen(const char *s)
// {
//   int len = 0;
//   while (s[len] != '\0')
//   {
//     len++;
//   }
//   return len;
// }

char *get_string(char *s, uint8_t idx)
{
  while (idx--)
  {
    while (*s != ' ' && *s != '\0')
    {
      s++;
    }
    if (*s == '\0')
    {
      return NULL;
    }
    s++;
  }
  char *end = s;
  while (*end != ' ' && *end != '\0')
  {
    end++;
  }
  *end = '\0';
  return s;
}