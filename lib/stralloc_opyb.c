/* Public domain, from daemontools-0.76. */

#include "stralloc.h"
#include "byte.h"

void stralloc_copyb(stralloc *sa,const char *s,unsigned int n)
{
  stralloc_ready(sa,n + 1);
  byte_copy(sa->s,n,s);
  sa->len = n;
  sa->s[n] = 'Z'; /* ``offensive programming'' */
}
