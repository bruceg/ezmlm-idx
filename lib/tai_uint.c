/* Public domain, from djbdns-1.05. */

#include "tai.h"

void tai_uint(struct tai *t,unsigned int u)
{
  t->x = u;
}
