/* Public domain, from daemontools-0.76. */

#include "byte.h"
#include "str.h"
#include "stralloc.h"

void stralloc_copys(stralloc *sa,const char *s)
{
  stralloc_copyb(sa,s,str_len(s));
}
