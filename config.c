/*$Id$*/

#include <unistd.h>
#include "config.h"
#include "die.h"
#include "errtxt.h"
#include "slurp.h"
#include "strerr.h"

stralloc key = {0};

void startup(const char *dir)
{
  if (dir == 0)
    die_usage();

  if (dir[0] != '/')
    strerr_die2x(100,FATAL,ERR_SLASH);

  if (chdir(dir) == -1)
    strerr_die4sys(111,FATAL,ERR_SWITCH,dir,": ");
}

void load_config(void)
{
  switch(slurp("key",&key,512)) {
    case -1:
      strerr_die3sys(111,FATAL,ERR_READ,"key: ");
    case 0:
      strerr_die3x(100,FATAL,"key",ERR_NOEXIST);
  }
}
