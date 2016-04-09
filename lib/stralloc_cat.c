/* Public domain, from daemontools-0.76. */

#include "byte.h"
#include "stralloc.h"

void stralloc_cat(stralloc *sato,const stralloc *safrom)
{
  stralloc_catb(sato,safrom->s,safrom->len);
}
