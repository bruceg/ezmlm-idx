#include "fork.h"
#include "errtxt.h"
#include "strerr.h"
#include "wrap.h"

int wrap_fork(const char *FATAL)
{
  int child;
  if ((child = fork()) == -1)
    strerr_die2sys(111, FATAL, ERR_FORK);
  return child;
}
