/*$Id$*/
#include "stralloc.h"
#include "fmt.h"
#include "subscribe.h"
#include "errtxt.h"
#include <unistd.h>
#include <libpq-fe.h>

extern PGconn *pgsql;

static stralloc logline = {0};
static char strnum[FMT_ULONG];

const char *logmsg(const char *dir,
		   unsigned long num,
		   unsigned long listno,
		   unsigned long subs,
		   int done)
/* creates an entry for message num and the list listno and code "done". */
/* Returns NULL on success, "" if dir/sql was not found, and the error   */
/* string on error.   NOTE: This routine does nothing for non-sql lists! */
{
  const char *table;
  const char *ret;

  PGresult *result;
  PGresult *result2;

  if ((ret = opensub(dir,0,&table))) {
    if (*ret)
      return ret;
    else
      return (char *) 0;	/* no SQL => success */
  }
  if (!stralloc_copys(&logline,"INSERT INTO ")) return ERR_NOMEM;
  if (!stralloc_cats(&logline,table)) return ERR_NOMEM;
  if (!stralloc_cats(&logline,"_mlog (msgnum,listno,subs,done) VALUES ("))
	return ERR_NOMEM;
  if (!stralloc_catb(&logline,strnum,fmt_ulong(strnum,num))) return ERR_NOMEM;
  if (!stralloc_cats(&logline,",")) return ERR_NOMEM;
  if (!stralloc_catb(&logline,strnum,fmt_ulong(strnum,listno)))
	return ERR_NOMEM;
  if (!stralloc_cats(&logline,",")) return ERR_NOMEM;
  if (!stralloc_catb(&logline,strnum,fmt_ulong(strnum,subs))) return ERR_NOMEM;
  if (!stralloc_cats(&logline,",")) return ERR_NOMEM;
  if (done < 0) {
    done = - done;
    if (!stralloc_append(&logline,"-")) return ERR_NOMEM;
  }
  if (!stralloc_catb(&logline,strnum,fmt_uint(strnum,done))) return ERR_NOMEM;
  if (!stralloc_append(&logline,")")) return ERR_NOMEM;

  if (!stralloc_0(&logline)) return ERR_NOMEM;
  result = PQexec(pgsql,logline.s);
  if(result==NULL)
    return (PQerrorMessage(pgsql));
  if(PQresultStatus(result) != PGRES_COMMAND_OK) { /* Check if duplicate */
    if (!stralloc_copys(&logline,"SELECT msgnum FROM ")) return ERR_NOMEM;
    if (!stralloc_cats(&logline,table)) return ERR_NOMEM;
    if (!stralloc_cats(&logline,"_mlog WHERE msgnum = ")) return ERR_NOMEM;
    if (!stralloc_catb(&logline,strnum,fmt_ulong(strnum,num)))
      return ERR_NOMEM;
    if (!stralloc_cats(&logline," AND listno = ")) return ERR_NOMEM;
    if (!stralloc_catb(&logline,strnum,fmt_ulong(strnum,listno)))
      return ERR_NOMEM;
    if (!stralloc_cats(&logline," AND done = ")) return ERR_NOMEM;
    if (!stralloc_catb(&logline,strnum,fmt_ulong(strnum,done)))
      return ERR_NOMEM;
    /* Query */
    if (!stralloc_0(&logline)) return ERR_NOMEM;
    result2 = PQexec(pgsql,logline.s);
    if (result2 == NULL)
      return (PQerrorMessage(pgsql));
    if (PQresultStatus(result2) != PGRES_TUPLES_OK)
      return (char *) (PQresultErrorMessage(result2));
    /* No duplicate, return ERROR from first query */
    if (PQntuples(result2)<1)
      return (char *) (PQresultErrorMessage(result));
    PQclear(result2);
  }
  PQclear(result);
  return (char *) 0;
}
