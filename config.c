/*$Id$*/

#include <unistd.h>
#include "strerr.h"
#include "errtxt.h"
#include "die.h"

void startup(const char *dir)
{
  if (dir == 0)
    die_usage();

  if (dir[0] != '/')
    strerr_die2x(100,FATAL,ERR_SLASH);

  if (chdir(dir) == -1)
    strerr_die4sys(111,FATAL,ERR_SWITCH,dir,": ");
}
