/*$Id$*/

#include "str.h"
#include "slurp.h"
#include "scan.h"
#include "stralloc.h"
#include "strerr.h"
#include "errtxt.h"
#include "subscribe.h"
#include <mysql.h>

static stralloc myp = {0};
static stralloc ers = {0};
static stralloc fn = {0};
static stralloc ourdb = {0};
static char *ourtable = (char *) 0;

const char *opensql(dbname,table)
/* reads the file dbname/sql, and if the file exists, parses it into the    */
/* components. The string should be host:port:user:pw:db:table.         If  */
/* the file does not exists, returns "". On success returns NULL. On error  */
/* returns error string for temporary error. If table is NULL it is         */
/* left alone. If *table is not null, it overrides the table in the sql     */
/* file. If we already opended dbname the cached info is used, rather than  */
/* rereading the file. Note that myp is static and all pointers point to it.*/
char *dbname;	/* database directory */
char **table;	/* table root_name */

{
  char *host = (char *) 0;
  unsigned long port = 0L;
  char *db = "ezmlm";		/* default */
  char *user = (char *) 0;
  char *pw = (char *) 0;
  unsigned int j;
  char *cp;

  if (!stralloc_copys(&fn,dbname)) return ERR_NOMEM;
  if (fn.len == ourdb.len && !str_diffn(ourdb.s,fn.s,fn.len)) {
    if (table) {
      if (*table) ourtable = *table;
      else *table = ourtable;
    }
    return 0;
  }
  if (!stralloc_cats(&fn,"/sql")) return ERR_NOMEM;
  if (!stralloc_0(&fn)) return ERR_NOMEM;
		/* host:port:db:table:user:pw:name */

  myp.len = 0;
  switch (slurp(fn.s,&myp,128)) {
	case -1:	if (!stralloc_copys(&ers,ERR_READ)) return ERR_NOMEM;
			if (!stralloc_cat(&ers,&fn)) return ERR_NOMEM;
			if (!stralloc_0(&ers)) return ERR_NOMEM;
			return ers.s;
	case 0: return "";
  }
  if (!stralloc_copy(&ourdb,&fn)) return ERR_NOMEM;
  if (!stralloc_append(&myp,"\n")) return ERR_NOMEM;
  for (j=0; j< myp.len; ++j) {
    if (myp.s[j] == '\n') { myp.s[j] = '\0'; break; }
  }
						/* get connection parameters */
  if (!stralloc_0(&myp)) return ERR_NOMEM;
  host = myp.s;
  if (myp.s[j = str_chr(myp.s,':')]) {
    cp = myp.s + j++;
    *(cp++) = '\0';
    scan_ulong(cp,&port);
    if (myp.s[j += str_chr(myp.s+j,':')]) {
      j++;
      user = myp.s + j;
      if (myp.s[j += str_chr(myp.s+j,':')]) {
        pw = myp.s + j++;
	*(pw++) = '\0';
	if (myp.s[j += str_chr(myp.s+j,':')]) {
          db = myp.s + j++;
	  *(db++) = '\0';
	  if (myp.s[j += str_chr(myp.s+j,':')]) {
	    ourtable = myp.s + j++;
	    *(ourtable++) = '\0';
	  }
	}
      }
    }
  }
  if (host && !*host) host = (char *) 0;
  if (user && !*user) user = (char *) 0;
  if (pw && !*pw) pw = (char *) 0;
  if (db && !*db) db = (char *) 0;
  if (ourtable && !*ourtable) ourtable = (char *) 0;
  if (table) {
    if (*table) ourtable = *table;
    else *table = ourtable;
    if (!*table) return ERR_NO_TABLE;
  }
  if (!psql) {
    if (!(psql = mysql_init((MYSQL *) 0)))
	 return ERR_NOMEM;					/* init */
    if (!(mysql_real_connect((MYSQL *) psql, host, user, pw, db,
	(unsigned int) port, 0, CLIENT_COMPRESS)))		/* conn */
		return mysql_error((MYSQL *) psql);
  }
  return (char *) 0;
}

void closesql()
/* close connection to SQL server, if open */
{
	if (psql) mysql_close((MYSQL *) psql);
	psql = (void *) 0;			/* destroy pointer */
	ourdb.len = 0;				/* destroy cache */
	return;
}

