/*$Id$*/
#include "stralloc.h"
#include "getln.h"
#include "readwrite.h"
#include "substdio.h"
#include "open.h"
#include "byte.h"
#include "case.h"
#include "strerr.h"
#include "error.h"
#include "uint32.h"
#include "fmt.h"
#include "subscribe.h"
#include "errtxt.h"
#include "die.h"
#include "idx.h"
#include <unistd.h>
#include <libpq-fe.h>

extern PGconn *pgsql;

static stralloc addr = {0};
static stralloc line = {0};

const char *issub(const char *dir,		/* directory to basedir */
		  const char *subdir,
		  const char *userhost)
/* Returns (char *) to match if userhost is in the subscriber database      */
/* dir, 0 otherwise. dir is a base directory for a list and may NOT         */
/* be NULL        */
/* NOTE: The returned pointer is NOT VALID after a subsequent call to issub!*/

{
  PGresult *result;
  const char *ret;
  const char *table;

  unsigned int j;

  if ((ret = opensub(dir,subdir,&table))) {
    if (*ret) strerr_die2x(111,FATAL,ret);

    return std_issub(dir,subdir,userhost);

  } else {						/* SQL version  */
	/* SELECT address FROM list WHERE address = 'userhost' AND hash */
	/* BETWEEN 0 AND 52. Without the hash restriction, we'd make it */
	/* even easier to defeat. Just faking sender to the list name would*/
	/* work. Since sender checks for posts are bogus anyway, I don't */
	/* know if it's worth the cost of the "WHERE ...". */

    if (!stralloc_copys(&addr,userhost)) die_nomem();
    j = byte_rchr(addr.s,addr.len,'@');
    if (j == addr.len) return 0;
    case_lowerb(addr.s + j + 1,addr.len - j - 1);

    if (!stralloc_copys(&line,"SELECT address FROM ")) die_nomem();
    if (!stralloc_cats(&line,table)) die_nomem();
    if (!stralloc_cats(&line," WHERE address ~* '^")) die_nomem();
    if (!stralloc_cat(&line,&addr)) die_nomem();
    if (!stralloc_cats(&line,"$'")) die_nomem();

    if (!stralloc_0(&line)) die_nomem();
    result = PQexec(pgsql,line.s);
    if (result == NULL)
      strerr_die2x(111,FATAL,PQerrorMessage(pgsql));
    if (PQresultStatus(result) != PGRES_TUPLES_OK )
      strerr_die2x(111,FATAL,PQresultErrorMessage(result));

    /* No data returned in QUERY */
    if (PQntuples(result) < 1)
      return (char *)0;

    if (!stralloc_copyb(&line,PQgetvalue(result,0,0),PQgetlength(result,0,0)))
	die_nomem();
    if (!stralloc_0(&line)) die_nomem();

    PQclear(result);
    return line.s;
  }
}

