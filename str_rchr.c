/* Public domain, from djbdns-1.05. */
/* As per http://cr.yp.to/djbdns/res-disaster.html */

#include "str.h"

unsigned int str_rchr(register const char *s,int c)
{
  register char ch;
  register const char *t;
  register const char *u;

  ch = c;
  t = s;
  u = 0;
  for (;;) {
    if (!*t) break; if (*t == ch) u = t; ++t;
    if (!*t) break; if (*t == ch) u = t; ++t;
    if (!*t) break; if (*t == ch) u = t; ++t;
    if (!*t) break; if (*t == ch) u = t; ++t;
  }
  if (!u) u = t;
  return u - s;
}
