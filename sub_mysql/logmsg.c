/*$Id: logmsg.c,v 1.10 1999/11/10 04:08:27 lindberg Exp $*/
/*$Name: ezmlm-idx-040 $*/
#include "stralloc.h"
#include "fmt.h"
#include "subscribe.h"
#include "errtxt.h"
#include <mysql.h>
#include <mysqld_error.h>

static stralloc logline = {0};
static char strnum[FMT_ULONG];

char *logmsg(dir,num,listno,subs,done)
/* creates an entry for message num and the list listno and code "done". */
/* Returns NULL on success, "" if dir/sql was not found, and the error   */
/* string on error.   NOTE: This routine does nothing for non-sql lists! */
char *dir;
unsigned long num;
unsigned long listno;
unsigned long subs;
int done;
{
  char *table = (char *) 0;
  char *ret;

  if ((ret = opensql(dir,&table))) {
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

  if (mysql_real_query((MYSQL *) psql,logline.s,logline.len))	/* log query */
    if (mysql_errno((MYSQL *) psql) != ER_DUP_ENTRY)	/* ignore dups */
	return mysql_error((MYSQL *) psql);
  return (char *) 0;
}
