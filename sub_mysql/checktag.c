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
#include <mysql.h>

extern MYSQL *mysql;

static stralloc line = {0};
static stralloc quoted = {0};
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
  MYSQL_RES *result;
  MYSQL_ROW row;
  const char *table;
  const char *r;

  if ((r = opensub(dir,&table))) {
    if (*r) return r;
    return std_checktag(dir,num,action,seed,hash);

  } else {

/* SELECT msgnum FROM table_cookie WHERE msgnum=num and cookie='hash' */
/* succeeds only is everything correct. 'hash' is quoted since it is  */
/*  potentially hostile. */
    if (listno) {			/* only for slaves */
      if (!stralloc_copys(&line,"SELECT listno FROM ")) return ERR_NOMEM;
      if (!stralloc_cats(&line,table)) return ERR_NOMEM;
      if (!stralloc_cats(&line,"_mlog WHERE listno=")) return ERR_NOMEM;
      if (!stralloc_catb(&line,strnum,fmt_ulong(strnum,listno)))
	return ERR_NOMEM;
      if (!stralloc_cats(&line," AND msgnum=")) return ERR_NOMEM;
      if (!stralloc_catb(&line,strnum,fmt_ulong(strnum,num))) return ERR_NOMEM;
      if (!stralloc_cats(&line," AND done > 3")) return ERR_NOMEM;
      if (mysql_real_query(mysql,line.s,line.len) != 0)
	return mysql_error(mysql);			/* query */
      if (!(result = mysql_use_result(mysql)))		/* use result */
	return mysql_error(mysql);
      if ((row = mysql_fetch_row(result)))
	return "";					/*already done */
      else						/* no result */
        if (!mysql_eof(result))
	  return mysql_error(mysql);
      mysql_free_result(result);			/* free res */
    }

    if (!stralloc_copys(&line,"SELECT msgnum FROM ")) return ERR_NOMEM;
    if (!stralloc_cats(&line,table)) return ERR_NOMEM;
    if (!stralloc_cats(&line,"_cookie WHERE msgnum=")) return ERR_NOMEM;
    if (!stralloc_catb(&line,strnum,fmt_ulong(strnum,num))) return ERR_NOMEM;
    if (!stralloc_cats(&line," and cookie='")) return ERR_NOMEM;
    if (!stralloc_ready(&quoted,COOKIE * 2 + 1)) return ERR_NOMEM;
    quoted.len = mysql_escape_string(quoted.s,hash,COOKIE);
    if (!stralloc_cat(&line,&quoted)) return ERR_NOMEM;
    if (!stralloc_cats(&line,"'")) return ERR_NOMEM;

    if (mysql_real_query(mysql,line.s,line.len) != 0)	/* select */
	return mysql_error(mysql);
    if (!(result = mysql_use_result(mysql)))
	return mysql_error(mysql);
    if (!mysql_fetch_row(result)) {
    if (!mysql_eof(result))		/* some error occurred */
	return mysql_error(mysql);
      mysql_free_result(result);	/* eof => query ok, but null result*/
      return "";			/* not parent => perm error */
    }
    mysql_free_result(result);		/* success! cookie matches */
    if (listno)
      (void) logmsg(dir,num,listno,0L,3);	/* non-ess mysql logging */
    return (char *)0;
  }
}
