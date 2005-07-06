/*$Id$*/

#include "slurp.h"
#include "byte.h"
#include "stralloc.h"
#include "scan.h"
#include "fmt.h"
#include "cookie.h"
#include "makehash.h"
#include "strerr.h"
#include "errtxt.h"
#include "subscribe.h"
#include <unistd.h>
#include <libpq-fe.h>

extern PGconn *pgsql;

static stralloc line = {0};
static char strnum[FMT_ULONG];

const char *checktag (const char *dir,		/* the db base dir */
		      unsigned long num,	/* message number */
		      unsigned long listno,	/* bottom of range => slave */
		      const char *action,
		      const char *seed,		/* cookie base */
		      const char *hash)		/* cookie */
/* reads dir/sql. If not present, returns success (NULL). If dir/sql is    */
/* present, checks hash against the cookie table. If match, returns success*/
/* (NULL), else returns "". If error, returns error string. */
{
  PGresult *result;
  /*  int row; */
  const char *table;
  const char *r;

  if ((r = opensub(dir,&table))) {
    if (*r) return r;
    return std_checktag(dir,num,action,seed,hash);

  } else {

    /* SELECT msgnum FROM table_cookie WHERE msgnum=num and cookie='hash' */
    /* succeeds only is everything correct. 'hash' is quoted since it is  */
    /* potentially hostile. */
    if (listno) {			/* only for slaves */
      if (!stralloc_copys(&line,"SELECT listno FROM ")) return ERR_NOMEM;
      if (!stralloc_cats(&line,table)) return ERR_NOMEM;
      if (!stralloc_cats(&line,"_mlog WHERE listno=")) return ERR_NOMEM;
      if (!stralloc_catb(&line,strnum,fmt_ulong(strnum,listno)))
	return ERR_NOMEM;
      if (!stralloc_cats(&line," AND msgnum=")) return ERR_NOMEM;
      if (!stralloc_catb(&line,strnum,fmt_ulong(strnum,num))) return ERR_NOMEM;
      if (!stralloc_cats(&line," AND done > 3")) return ERR_NOMEM;

      if (!stralloc_0(&line)) return ERR_NOMEM;
      result = PQexec( pgsql, line.s );
      if(result == NULL)
	return (PQerrorMessage(pgsql));
      if( PQresultStatus(result) != PGRES_TUPLES_OK)
	return (char *) (PQresultErrorMessage(result));
      if( PQntuples(result) > 0 ) {
	PQclear(result);
	return("");
      } else
	PQclear(result);
    }

    if (!stralloc_copys(&line,"SELECT msgnum FROM ")) return ERR_NOMEM;
    if (!stralloc_cats(&line,table)) return ERR_NOMEM;
    if (!stralloc_cats(&line,"_cookie WHERE msgnum=")) return ERR_NOMEM;
    if (!stralloc_catb(&line,strnum,fmt_ulong(strnum,num))) return ERR_NOMEM;
    if (!stralloc_cats(&line," and cookie='")) return ERR_NOMEM;
    if (!stralloc_catb(&line,strnum,fmt_str(strnum,hash))) return ERR_NOMEM;
    if (!stralloc_cats(&line,"'")) return ERR_NOMEM;

    if (!stralloc_0(&line)) return ERR_NOMEM;
    result = PQexec(pgsql,line.s);
    if (result == NULL)
      return (PQerrorMessage(pgsql));
    if (PQresultStatus(result) != PGRES_TUPLES_OK)
      return (char *) (PQresultErrorMessage(result));
    if(PQntuples(result) < 0) {
      PQclear( result );
      return("");
    }

    PQclear(result);
    if (listno)
      (void) logmsg(dir,num,listno,0L,3);	/* non-ess mysql logging */
    return (char *)0;
  }
}
