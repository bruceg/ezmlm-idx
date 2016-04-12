/* Public domain, from djbdns-1.05. */
/* As per http://cr.yp.to/djbdns/res-disaster.html */

#include "stralloc.h"

void stralloc_catulong0(stralloc *sa,unsigned long u,unsigned int n)
{
  unsigned int len;
  unsigned long q;
  char *s;

  len = 1;
  q = u;
  while (q > 9) { ++len; q /= 10; }
  if (len < n) len = n;

  stralloc_readyplus(sa,len);
  s = sa->s + sa->len;
  sa->len += len;
  while (len) { s[--len] = '0' + (u % 10); u /= 10; }
}

void stralloc_catlong0(stralloc *sa,long l,unsigned int n)
{
  if (l < 0) {
    stralloc_append(sa,'-');
    l = -l;
  }
  stralloc_catulong0(sa,l,n);
}
