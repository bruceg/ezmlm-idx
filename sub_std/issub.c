/*$Id$*/

#include "subscribe.h"

const char *issub(const char *dir,		/* directory to basedir */
		  const char *subdir,
		  const char *userhost)
{
  return std_issub(dir,subdir,userhost);
}
