#include "errtxt.h"
#include "strerr.h"
#include "wrap.h"

void wrap_exitcode(int pid, const char *FATAL)
{
  switch (wrap_waitpid(pid, FATAL)) {
    case 100:
      strerr_die2x(100,FATAL,ERR_CHILD_FATAL);
    case 111:
      strerr_die2x(111,FATAL,ERR_CHILD_TEMP);
    case 0:
      break;
    default:
      strerr_die2x(111,FATAL,ERR_CHILD_UNKNOWN);
  }
}
