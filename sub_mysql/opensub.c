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

const char *opensub(const char *dir,
		    const char *subdir,
		    const char **table)
{
  struct sqlinfo info;
  const char *err;
  unsigned long portnum = 0L;

  if ((err = parsesql(dir,subdir,&info)) != 0)
    return err;
  *table = info.table;
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
