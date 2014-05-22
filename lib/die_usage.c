#include "strerr.h"
#include "messages.h"
#include "die.h"

void die_usage(void)
{
  strerr_die1x(100,USAGE);
}
