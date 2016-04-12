/* Public domain, from daemontools-0.76. */

#include "byte.h"

unsigned int byte_chr(const void *s,unsigned int n,int c)
{
  const void *p;
  return (p = memchr(s,c,n)) == 0 ? n : (p - s);
}
