/*$Id$*/
#include "stralloc.h"
#include "fmt.h"
#include "subscribe.h"
#include "errtxt.h"
#include <mysql.h>
#include <mysqld_error.h>

extern MYSQL *mysql;

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
  const char *table = (char *) 0;
  const char *ret;

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

  if (mysql_real_query(mysql,logline.s,logline.len))	/* log query */
    if (mysql_errno(mysql) != ER_DUP_ENTRY)	/* ignore dups */
	return mysql_error(mysql);
  return (char *) 0;
}
