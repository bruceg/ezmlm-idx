#include "fork.h"
#include "messages.h"
#include "strerr.h"
#include "wrap.h"
#include "die.h"
#include "idx.h"

int wrap_fork(void)
{
  int child;
  if ((child = fork()) == -1)
    strerr_die2sys(111, FATAL, MSG(ERR_FORK));
  return child;
}
