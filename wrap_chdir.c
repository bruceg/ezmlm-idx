#include <unistd.h>
#include "msgtxt.h"
#include "strerr.h"
#include "wrap.h"
#include "die.h"

void wrap_chdir(const char *dir)
{
  if (chdir(dir) == -1)
    strerr_die4sys(111,FATAL,ERR_SWITCH,dir,": ");
}
