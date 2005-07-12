/*$Id$*/
#include "stralloc.h"
#include "strerr.h"
#include "errtxt.h"
#include "subscribe.h"

const char *opensub(const char *dir,	/* database directory */
		    const char *subdir,
		    const char **table)	/* table root_name */
{
  return 0;
  (void)dir;
  (void)subdir;
  (void)table;
}

void closesub(void)
/* close connection to SQL server, if open */
{
  return;
}

