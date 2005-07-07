/*$Id$*/

#include <stdio.h>
#include <unistd.h>
#include "str.h"
#include "stralloc.h"
#include "getln.h"
#include "readwrite.h"
#include "substdio.h"
#include "strerr.h"
#include "open.h"
#include "byte.h"
#include "case.h"
#include "lock.h"
#include "error.h"
#include "subscribe.h"
#include "uint32.h"
#include "subhash.h"
#include "fmt.h"
#include "errtxt.h"
#include "log.h"
#include "die.h"
#include "idx.h"
#include <mysql.h>
#include <mysqld_error.h>

extern MYSQL *mysql;

static stralloc addr = {0};
static stralloc lcaddr = {0};
static stralloc line = {0};
static stralloc domain = {0};
static stralloc logline = {0};
static stralloc quoted = {0};

int subscribe(const char *dbname,
	      const char *userhost,
	      int flagadd,
	      const char *comment,
	      const char *event,
	      int flagmysql,
	      int forcehash)
/* add (flagadd=1) or remove (flagadd=0) userhost from the subscr. database  */
/* dbname. Comment is e.g. the subscriber from line or name. It is added to  */
/* the log. Event is the action type, e.g. "probe", "manual", etc. The       */
/* direction (sub/unsub) is inferred from flagadd. Returns 1 on success, 0   */
/* on failure. If flagmysql is set and the file "sql" is found in the        */
/* directory dbname, it is parsed and a mysql db is assumed. if forcehash is */
/* >=0 it is used in place of the calculated hash. This makes it possible to */
/* add addresses with a hash that does not exist. forcehash has to be 0..99. */
/* for unsubscribes, the address is only removed if forcehash matches the    */
/* actual hash. This way, ezmlm-manage can be prevented from touching certain*/
/* addresses that can only be removed by ezmlm-unsub. Usually, this would be */
/* used for sublist addresses (to avoid removal) and sublist aliases (to     */
/* prevent users from subscribing them (although the cookie mechanism would  */
/* prevent the resulting duplicate message from being distributed. */

{
  MYSQL_RES *result;
  MYSQL_ROW row;
  char *cpat;
  char szhash[3] = "00";
  const char *r = (char *) 0;
  const char *table;

  unsigned int j;
  unsigned char ch;

  if (userhost[str_chr(userhost,'\n')])
    strerr_die2x(100,FATAL,ERR_ADDR_NL);

  if (!flagmysql || (r = opensub(dbname,&table))) {
    if (r && *r) strerr_die2x(111,FATAL,r);

    return std_subscribe(dbname,userhost,flagadd,comment,event,forcehash);

  } else {				/* SQL version */
    domain.len = 0;			/* clear domain */
					/* lowercase and check address */
    if (!stralloc_copys(&addr,userhost)) die_nomem();
    if (addr.len > 255)			/* this is 401 in std ezmlm. 255 */
					/* should be plenty! */
      strerr_die2x(100,FATAL,ERR_ADDR_LONG);
    j = byte_rchr(addr.s,addr.len,'@');
    if (j == addr.len)
      strerr_die2x(100,FATAL,ERR_ADDR_AT);
    cpat = addr.s + j;
    case_lowerb(cpat + 1,addr.len - j - 1);
    if (!stralloc_ready(&quoted,2 * addr.len + 1)) die_nomem();
    quoted.len = mysql_escape_string(quoted.s,addr.s,addr.len);
	/* stored unescaped, so it should be ok if quoted.len is >255, as */
	/* long as addr.len is not */

    if (forcehash < 0) {
      if (!stralloc_copy(&lcaddr,&addr)) die_nomem();
      case_lowerb(lcaddr.s,j);		/* make all-lc version of address */
      ch = subhashsa(&lcaddr);
    } else
      ch = (forcehash % 100);

    szhash[0] = '0' + ch / 10;		/* hash for sublist split */
    szhash[1] = '0' + (ch % 10);

    if (flagadd) {
      if (!stralloc_copys(&line,"LOCK TABLES ")) die_nomem();
      if (!stralloc_cats(&line,table)) die_nomem();
      if (!stralloc_cats(&line," WRITE")) die_nomem();
      if (mysql_real_query(mysql,line.s,line.len))
	strerr_die2x(111,FATAL,mysql_error(mysql));
      if (!stralloc_copys(&line,"SELECT address FROM ")) die_nomem();
      if (!stralloc_cats(&line,table)) die_nomem();
      if (!stralloc_cats(&line," WHERE address='")) die_nomem();
      if (!stralloc_cat(&line,&quoted)) die_nomem();	/* addr */
      if (!stralloc_cats(&line,"'")) die_nomem();
      if (mysql_real_query(mysql,line.s,line.len))
	strerr_die2x(111,FATAL,mysql_error(mysql));
      if (!(result = mysql_use_result(mysql)))
	strerr_die2x(111,FATAL,mysql_error(mysql));
      if ((row = mysql_fetch_row(result))) {			/* there */
	while (mysql_fetch_row(result));			/* use'm up */
	mysql_free_result(result);
	if (mysql_query(mysql,"UNLOCK TABLES"))
	  strerr_die2x(111,"FATAL",mysql_error(mysql));
        return 0;						/* there */
      } else {							/* not there */
	mysql_free_result(result);
	if (mysql_errno(mysql))			/* or ERROR */
	  strerr_die2x(111,FATAL,mysql_error(mysql));
	if (!stralloc_copys(&line,"INSERT INTO ")) die_nomem();
	if (!stralloc_cats(&line,table)) die_nomem();
	if (!stralloc_cats(&line," (address,hash) VALUES ('"))
		die_nomem();
	if (!stralloc_cat(&line,&quoted)) die_nomem();	/* addr */
	if (!stralloc_cats(&line,"',")) die_nomem();
	if (!stralloc_cats(&line,szhash)) die_nomem();	/* hash */
	if (!stralloc_cats(&line,")")) die_nomem();
        if (mysql_real_query(mysql,line.s,line.len))	/* INSERT */
	  strerr_die2x(111,FATAL,mysql_error(mysql));
	if (mysql_query(mysql,"UNLOCK TABLES"))
	  strerr_die2x(111,FATAL,mysql_error(mysql));
      }
    } else {							/* unsub */
      if (!stralloc_copys(&line,"DELETE FROM ")) die_nomem();
      if (!stralloc_cats(&line,table)) die_nomem();
      if (!stralloc_cats(&line," WHERE address='")) die_nomem();
      if (!stralloc_cat(&line,&quoted)) die_nomem();	/* addr */
      if (forcehash >= 0) {
	if (!stralloc_cats(&line,"' AND hash=")) die_nomem();
	if (!stralloc_cats(&line,szhash)) die_nomem();
      } else {
        if (!stralloc_cats(&line,"' AND hash BETWEEN 0 AND 52"))
		die_nomem();
      }
      if (mysql_real_query(mysql,line.s,line.len))
	  strerr_die2x(111,FATAL,mysql_error(mysql));
      if (mysql_affected_rows(mysql) == 0)
	return 0;				/* address wasn't there*/
    }

		/* log to subscriber log */
		/* INSERT INTO t_slog (address,edir,etype,fromline) */
		/* VALUES('address',{'+'|'-'},'etype','[comment]') */

    if (!stralloc_copys(&logline,"INSERT INTO ")) die_nomem();
    if (!stralloc_cats(&logline,table)) die_nomem();
    if (!stralloc_cats(&logline,
	"_slog (address,edir,etype,fromline) VALUES ('")) die_nomem();
    if (!stralloc_cat(&logline,&quoted)) die_nomem();
    if (flagadd) {						/* edir */
      if (!stralloc_cats(&logline,"','+','")) die_nomem();
    } else {
      if (!stralloc_cats(&logline,"','-','")) die_nomem();
    }
    if (*(event + 1))	/* ezmlm-0.53 uses '' for ezmlm-manage's work */
      if (!stralloc_catb(&logline,event+1,1)) die_nomem();	/* etype */
    if (!stralloc_cats(&logline,"','")) die_nomem();
    if (comment && *comment) {
	j = str_len(comment);
	if (!stralloc_ready(&quoted,2 * j + 1)) die_nomem();
	quoted.len = mysql_escape_string(quoted.s,comment,j);	/* from */
	if (!stralloc_cat(&logline,&quoted)) die_nomem();
    }
    if (!stralloc_cats(&logline,"')")) die_nomem();

    if (mysql_real_query(mysql,logline.s,logline.len))
		;				/* log (ignore errors) */
    if (!stralloc_0(&addr))
		;				/* ignore errors */
    logaddr(dbname,event,addr.s,comment);	/* also log to old log */
    return 1;					/* desired effect */
  }
}
