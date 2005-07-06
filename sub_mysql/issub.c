/*$Id$*/

#include <unistd.h>
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
#include <mysql.h>

extern MYSQL *mysql;

static stralloc addr = {0};
static stralloc line = {0};
static stralloc quoted = {0};

const char *issub(const char *dbname,		/* directory to basedir */
		  const char *userhost)
/* Returns (char *) to match if userhost is in the subscriber database     */
/* dbname, 0 otherwise. dbname is a base directory for a list and may NOT  */
/* be NULL        */
/* NOTE: The returned pointer is NOT VALID after a subsequent call to issub!*/

{
  MYSQL_RES *result;
  MYSQL_ROW row;
  const char *ret;
  const char *table;
  unsigned long *lengths;

  unsigned int j;

  if ((ret = opensub(dbname,&table))) {
    if (*ret) strerr_die2x(111,FATAL,ret);

    return std_issub(dbname,userhost);

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
    if (!stralloc_cats(&line," WHERE address = '")) die_nomem();
    if (!stralloc_ready(&quoted,2 * addr.len + 1)) die_nomem();
    if (!stralloc_catb(&line,quoted.s,
	mysql_escape_string(quoted.s,userhost,addr.len))) die_nomem();
    if (!stralloc_cats(&line,"'"))
		die_nomem();
    if (mysql_real_query(mysql,line.s,line.len))	/* query */
		strerr_die2x(111,FATAL,mysql_error(mysql));
    if (!(result = mysql_use_result(mysql)))
		strerr_die2x(111,FATAL,mysql_error(mysql));
    row = mysql_fetch_row(result);
    ret = (char *) 0;
    if (!row) {		/* we need to return the actual address as other */
			/* dbs may accept user-*@host, but we still want */
			/* to make sure to send to e.g the correct moderator*/
			/* address. */
      if (!mysql_eof(result))
		strerr_die2x(111,FATAL,mysql_error(mysql));
    } else {
      if (!(lengths = mysql_fetch_lengths(result)))
		strerr_die2x(111,FATAL,mysql_error(mysql));
      if (!stralloc_copyb(&line,row[0],lengths[0])) die_nomem();
      if (!stralloc_0(&line)) die_nomem();
      ret = line.s;
      while ((row = mysql_fetch_row(result)));	/* maybe not necessary */
      mysql_free_result(result);
    }
    return ret;
  }
}
