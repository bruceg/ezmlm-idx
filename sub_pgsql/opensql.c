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

const char *opensql(const char *dbname,	/* database directory */
		    const char **table)	/* table root_name */
/* reads the file dbname/sql, and if the file exists, parses it into the    */
/* components. The string should be host:port:user:pw:db:table.         If  */
/* the file does not exists, returns "". On success returns NULL. On error  */
/* returns error string for temporary error. If table is NULL it is         */
/* left alone. If *table is not null, it overrides the table in the sql     */
/* file. If we already opended dbname the cached info is used, rather than  */
/* rereading the file. Note that myp is static and all pointers point to it.*/
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

void closesql(void)
/* close connection to SQL server, if open */
{
  if (pgsql)
    PQfinish(pgsql);
  pgsql = 0;		/* Destroy pointer */
  return;
}
