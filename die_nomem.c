#include "strerr.h"
#include "messages.h"
#include "die.h"

void die_nomem(void)
{
  strerr_die2x(100,FATAL,"out of memory");
}
