/* Public domain, from djbdns-1.05. */
/* As per http://cr.yp.to/djbdns/res-disaster.html */

#include "case.h"

int case_diffb(const char *s,unsigned int len,const char *t)
{
  unsigned char x;
  unsigned char y;

  while (len > 0) {
    --len;
    x = *s++ - 'A';
    if (x <= 'Z' - 'A') x += 'a'; else x += 'A';
    y = *t++ - 'A';
    if (y <= 'Z' - 'A') y += 'a'; else y += 'A';
    if (x != y)
      return ((int)(unsigned int) x) - ((int)(unsigned int) y);
  }
  return 0;
}
