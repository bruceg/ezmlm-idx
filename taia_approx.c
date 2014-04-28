/* Public domain, from djbdns-1.05. */

#include "taia.h"

double taia_approx(const struct taia *t)
{
  return tai_approx(&t->sec) + taia_frac(t);
}
