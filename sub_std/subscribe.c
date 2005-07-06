/*$Id$*/

#include "subscribe.h"

int subscribe(const char *dbname,
	      const char *userhost,
	      int flagadd,
	      const char *comment,
	      const char *event,
	      int flagmysql,
	      int forcehash)
{
  return std_subscribe(dbname,userhost,flagadd,comment,event,forcehash);
  (void)flagmysql;
}
