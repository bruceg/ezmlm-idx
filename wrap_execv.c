#include "error.h"
#include "errtxt.h"
#include "strerr.h"
#include "wrap.h"

static void wrapper(int (*fn)(char*,char**),
		    const char **args, const char *FATAL)
{
  fn((char*)*args, (char**)args);
  strerr_die4sys((errno == error_txtbsy
                 || errno == error_nomem
		 || errno == error_io
		 ) ? 111 : 100, FATAL, ERR_EXECUTE, args[0], ": ");
}

extern int execv(char*,char**);
extern int execvp(char*,char**);

void wrap_execv(const char **args, const char *FATAL)
{
  wrapper(execv, args, FATAL);
}

void wrap_execvp(const char **args, const char *FATAL)
{
  wrapper(execvp, args, FATAL);
}
