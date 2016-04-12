/* Public domain, from daemontools-0.76. */

#include "str.h"

int str_start(const char *s,const char *t)
{
  char x;

  for (;;) {
    x = *t++; if (!x) return 1; if (x != *s++) return 0;
    x = *t++; if (!x) return 1; if (x != *s++) return 0;
    x = *t++; if (!x) return 1; if (x != *s++) return 0;
    x = *t++; if (!x) return 1; if (x != *s++) return 0;
  }
}
