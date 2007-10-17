#include "error.h"
#include "msgtxt.h"
#include "strerr.h"
#include "wrap.h"
#include "die.h"
#include "idx.h"

static void wrapper(int (*fn)(char*,char**), const char **args)
{
  fn((char*)*args, (char**)args);
  strerr_die2sys((errno == error_txtbsy
                 || errno == error_nomem
		 || errno == error_io
		 ) ? 111 : 100, FATAL, MSG1("ERR_EXECUTE",args[0]));
}

extern int execv(char*,char**);
extern int execvp(char*,char**);

void wrap_execv(const char **args)
{
  wrapper(execv, args);
}

void wrap_execvp(const char **args)
{
  wrapper(execvp, args);
}
