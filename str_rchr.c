/* Public domain, from djbdns-1.05. */
/* As per http://cr.yp.to/djbdns/res-disaster.html */

#include "str.h"

unsigned int str_rchr(const char *s,int c)
{
  char ch;
  const char *t;
  const char *u;

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
