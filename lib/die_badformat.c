/*$Id$*/

#include "strerr.h"
#include "errtxt.h"
#include "die.h"

void die_badformat(void)
{
  strerr_die2x(100,FATAL,ERR_BAD_REQUEST);
}
