/* Public domain, from daemontools-0.76. */

#include "byte.h"
#include "str.h"
#include "stralloc.h"

int stralloc_cats(stralloc *sa,const char *s)
{
  return stralloc_catb(sa,s,str_len(s));
}
