#include "strerr.h"
#include "messages.h"
#include "die.h"

void die_nomem(void)
{
  strerr_die2x(111,FATAL,"out of memory");
}
