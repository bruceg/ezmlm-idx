/* Public domain, from daemontools-0.76. */

#include "str.h"

unsigned int str_len(const char *s)
{
  const char *t;

  t = s;
  for (;;) {
    if (!*t) return t - s; ++t;
    if (!*t) return t - s; ++t;
    if (!*t) return t - s; ++t;
    if (!*t) return t - s; ++t;
  }
}
