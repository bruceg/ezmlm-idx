#include "wait.h"
#include "errtxt.h"
#include "strerr.h"
#include "wrap.h"

int wrap_waitpid(int pid, const char *FATAL)
{
  int wstat;
  wait_pid(&wstat,pid);
  if (wait_crashed(wstat))
    strerr_die2x(111,FATAL,ERR_CHILD_CRASHED);
  return wait_exitcode(wstat);
}
