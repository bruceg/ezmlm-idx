/* Public domain, from djbdns-1.05. */

#include "tai.h"

void tai_sub(struct tai *t,const struct tai *u,const struct tai *v)
{
  t->x = u->x - v->x;
}
