/* Public domain, from daemontools-0.76. */

#include "fmt.h"

unsigned int fmt_uint(char *s,unsigned int u)
{
  return fmt_ulong(s,u);
}
