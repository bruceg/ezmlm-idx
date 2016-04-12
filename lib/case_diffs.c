/* Public domain, from djbdns-1.05. */
/* As per http://cr.yp.to/djbdns/res-disaster.html */

#include "case.h"

int case_diffs(const char *s,const char *t)
{
  unsigned char x;
  unsigned char y;

  for (;;) {
    x = *s++ - 'A';
    if (x <= 'Z' - 'A') x += 'a'; else x += 'A';
    y = *t++ - 'A';
    if (y <= 'Z' - 'A') y += 'a'; else y += 'A';
    if (x != y) break;
    if (!x) break;
  }
  return ((int)(unsigned int) x) - ((int)(unsigned int) y);
}
