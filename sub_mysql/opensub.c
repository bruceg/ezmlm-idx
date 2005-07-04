/*$Id$*/

#include "str.h"
#include "slurp.h"
#include "scan.h"
#include "stralloc.h"
#include "strerr.h"
#include "errtxt.h"
#include "subscribe.h"
#include <mysql.h>

MYSQL *mysql = 0;

const char *opensub(const char *dbname,	/* database directory */
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
  unsigned long portnum = 0L;

  if ((err = parsesql(dbname,table,&info)) != 0)
    return err;
  if (info.port != 0)
    scan_ulong(info.port,&portnum);

  if (!mysql) {
    if (!(mysql = mysql_init((MYSQL *) 0)))
	 return ERR_NOMEM;					/* init */
    if (!(mysql_real_connect(mysql, info.host, info.user, info.pw, info.db,
	(unsigned int) portnum, 0, CLIENT_COMPRESS)))		/* conn */
		return mysql_error(mysql);
  }
  return (char *) 0;
}

void closesub(void)
/* close connection to SQL server, if open */
{
  if (mysql)
    mysql_close(mysql);
  mysql = 0;					/* destroy pointer */
  return;
}
