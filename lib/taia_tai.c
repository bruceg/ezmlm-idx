/* Public domain, from djbdns-1.05. */

#include "taia.h"

void taia_tai(const struct taia *ta,struct tai *t)
{
  *t = ta->sec;
}
