/* Public domain, from daemontools-0.76. */

#include "byte.h"

unsigned int byte_rchr(const void *s,unsigned int n,int c)
{
  const void *p;
  return (p = memrchr(s,c,n)) == 0 ? n : (p - s);
}
