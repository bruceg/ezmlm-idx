#include <unistd.h>
#include "messages.h"
#include "strerr.h"
#include "wrap.h"
#include "die.h"

void wrap_chdir(const char *dir)
{
  if (chdir(dir) == -1)
    strerr_die2sys(111,FATAL,MSG1(ERR_CHDIR,dir));
}
