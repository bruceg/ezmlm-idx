/* Public domain, from djbdns-1.05. */
/* As per http://cr.yp.to/djbdns/res-disaster.html */

#include "byte.h"
#include "stralloc.h"

int stralloc_copy(stralloc *sato,const stralloc *safrom)
{
  return stralloc_copyb(sato,safrom->s,safrom->len);
}
