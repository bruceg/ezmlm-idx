/*$Id$*/

#include "str.h"
#include "slurp.h"
#include "stralloc.h"
#include "strerr.h"
#include "errtxt.h"
#include "subscribe.h"
#include <unistd.h>
#include <libpq-fe.h> 

PGconn *pgsql = 0;

const char *opensub(const char *dbname,	/* database directory */
		    const char **table)	/* table root_name */
{
  struct sqlinfo info;
  const char *err;
  
  if ((err = parsesql(dbname,table,&info)) != 0)
    return err;

  if (!pgsql) {
    /* Make connection to database */
    pgsql = PQsetdbLogin(info.host,info.port,NULL,NULL,
			 info.db,info.user,info.pw);
    /* Check  to see that the backend connection was successfully made */
    if (PQstatus(pgsql) == CONNECTION_BAD)
      return PQerrorMessage(pgsql);
  }
  return (char *) 0;
}

void closesub(void)
/* close connection to SQL server, if open */
{
  if (pgsql)
    PQfinish(pgsql);
  pgsql = 0;		/* Destroy pointer */
  return;
}
