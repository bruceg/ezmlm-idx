/*$Id$*/

#include "subscribe.h"

const char *issub(const char *dbname,		/* directory to basedir */
		  const char *userhost)
{
  return std_issub(dbname,userhost);
}
