/* Public domain, from daemontools-0.76. */

#include "byte.h"

void byte_copy(char *to,unsigned int n,const char *from)
{
  for (;;) {
    if (!n) return; *to++ = *from++; --n;
    if (!n) return; *to++ = *from++; --n;
    if (!n) return; *to++ = *from++; --n;
    if (!n) return; *to++ = *from++; --n;
  }
}
