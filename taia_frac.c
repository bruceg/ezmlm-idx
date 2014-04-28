/* Public domain, from djbdns-1.05. */

#include "taia.h"

double taia_frac(const struct taia *t)
{
  return (t->atto * 0.000000001 + t->nano) * 0.000000001;
}
