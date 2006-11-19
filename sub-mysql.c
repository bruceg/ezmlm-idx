/*$Id$*/

#include "byte.h"
#include "case.h"
#include "cookie.h"
#include "date822fmt.h"
#include "datetime.h"
#include "die.h"
#include "error.h"
#include "errtxt.h"
#include "fmt.h"
#include "getln.h"
#include "idx.h"
#include "lock.h"
#include "log.h"
#include "makehash.h"
#include "open.h"
#include "qmail.h"
#include "readwrite.h"
#include "scan.h"
#include "slurp.h"
#include "sqllib.h"
#include "str.h"
#include "stralloc.h"
#include "strerr.h"
#include "sub_std.h"
#include "subhash.h"
#include "subscribe.h"
#include "substdio.h"
#include "uint32.h"
#include <mysql.h>
#include <mysqld_error.h>
#include <stdio.h>
#include <unistd.h>

static MYSQL *mysql = 0;

static char strnum[FMT_ULONG];
static stralloc addr = {0};
static stralloc domain = {0};
static stralloc lcaddr = {0};
static stralloc line = {0};
static stralloc logline = {0};
static stralloc quoted = {0};
static struct sqlinfo info;

static void die_write(void)
{
  strerr_die3x(111,FATAL,ERR_WRITE,"stdout");
}

static void _closesub(void)
/* close connection to SQL server, if open */
{
  if (mysql)
    mysql_close(mysql);
  mysql = 0;					/* destroy pointer */
  return;
}

static const char *_opensub(const char *dir,
			    const char *subdir)
{
  const char *err;
  unsigned long portnum = 0L;

  if ((err = parsesql(dir,subdir,&info)) != 0)
    return err;

  if (!mysql) {
    if (!(mysql = mysql_init((MYSQL *) 0)))
	 return ERR_NOMEM;					/* init */
    if (!(mysql_real_connect(mysql, info.host, info.user, info.pw, info.db,
	(unsigned int) portnum, 0, CLIENT_COMPRESS)))		/* conn */
		return mysql_error(mysql);
  }
  return (char *) 0;
}

static void safe_query(const stralloc* query)
{
  if (mysql_real_query(mysql,query->s,query->len))
    strerr_die2x(111,FATAL,mysql_error(mysql));
}

static MYSQL_RES *safe_select(const stralloc* query)
{
  MYSQL_RES *result;
  safe_query(query);
  if ((result = mysql_use_result(mysql)) == 0)
    strerr_die2x(111,FATAL,mysql_error(mysql));
  return result;
}

static const char *_checktag (const char *dir,		/* the db base dir */
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

/* SELECT msgnum FROM table_cookie WHERE msgnum=num and cookie='hash' */
/* succeeds only is everything correct. 'hash' is quoted since it is  */
/*  potentially hostile. */
    if (listno) {			/* only for slaves */
      if (!stralloc_copys(&line,"SELECT listno FROM ")) return ERR_NOMEM;
      if (!stralloc_cats(&line,info.table)) return ERR_NOMEM;
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
    if (!stralloc_cats(&line,info.table)) return ERR_NOMEM;
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

static const char *_issub(const char *dir,
			  const char *subdir,
			  const char *userhost)
/* Returns (char *) to match if userhost is in the subscriber database      */
/* dir, 0 otherwise. dir is a base directory for a list and may NOT         */
/* be NULL        */
/* NOTE: The returned pointer is NOT VALID after a subsequent call to issub!*/

{
  MYSQL_RES *result;
  MYSQL_ROW row;
  unsigned long *lengths;
  const char *ret;

  unsigned int j;

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
    if (!stralloc_cats(&line,info.table)) die_nomem();
    if (!stralloc_cats(&line," WHERE address = '")) die_nomem();
    if (!stralloc_ready(&quoted,2 * addr.len + 1)) die_nomem();
    if (!stralloc_catb(&line,quoted.s,
	mysql_escape_string(quoted.s,userhost,addr.len))) die_nomem();
    if (!stralloc_cats(&line,"'"))
		die_nomem();
    result = safe_select(&line);
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

static const char *_logmsg(const char *dir,
			   unsigned long num,
			   unsigned long listno,
			   unsigned long subs,
			   int done)
/* creates an entry for message num and the list listno and code "done". */
/* Returns NULL on success, "" if dir/sql was not found, and the error   */
/* string on error.   NOTE: This routine does nothing for non-sql lists! */
{
  if (!stralloc_copys(&logline,"INSERT INTO ")) return ERR_NOMEM;
  if (!stralloc_cats(&logline,info.table)) return ERR_NOMEM;
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
  return 0;
}

static unsigned long _putsubs(const char *dir,
			      const char *subdir,
			      unsigned long hash_lo,
			      unsigned long hash_hi,
			      int subwrite())		/* write function. */

/* Outputs all userhostesses in 'dbname' to stdout. If userhost is not null */
/* that userhost is excluded. 'dbname' is the base directory name. For the  */
/* mysql version, dbname is the directory where the file "sql" with mysql   */
/* access info is found. If this file is not present or if flagmysql is not */
/* set, the routine falls back to the old database style. subwrite must be a*/
/* function returning >=0 on success, -1 on error, and taking arguments     */
/* (char* string, unsigned int length). It will be called once per address  */
/* and should take care of newline or whatever needed for the output form.  */

{
  MYSQL_RES *result;
  MYSQL_ROW row;
  unsigned long *lengths;

  unsigned long no = 0L;

						/* main query */
    if (!stralloc_copys(&line,"SELECT address FROM "))
		die_nomem();
    if (!stralloc_cats(&line,info.table)) die_nomem();
    if (!stralloc_cats(&line," WHERE hash BETWEEN ")) die_nomem();
    if (!stralloc_catb(&line,strnum,fmt_ulong(strnum,hash_lo)))
		die_nomem();
    if (!stralloc_cats(&line," AND ")) die_nomem();
    if (!stralloc_catb(&line,strnum,fmt_ulong(strnum,hash_hi)))
		die_nomem();
    result = safe_select(&line);
    no = 0;
    while ((row = mysql_fetch_row(result))) {
	/* this is safe even if someone messes with the address field def */
    if (!(lengths = mysql_fetch_lengths(result)))
	strerr_die2x(111,FATAL,mysql_error(mysql));
      if (subwrite(row[0],lengths[0]) == -1) die_write();
      no++;					/* count for list-list fxn */
    }
    if (!mysql_eof(result))
	strerr_die2x(111,FATAL,mysql_error(mysql));
    mysql_free_result(result);
    return no;
}

static void _searchlog(const char *dir,		/* work directory */
		       const char *subdir,
		       char *search,		/* search string */
		       int subwrite())		/* output fxn */
/* opens dir/Log, and outputs via subwrite(s,len) any line that matches   */
/* search. A '_' is search is a wildcard. Any other non-alphanum/'.' char */
/* is replaced by a '_'. mysql version. Falls back on "manual" search of  */
/* local Log if no mysql connect info. */

{
  MYSQL_RES *result;
  MYSQL_ROW row;
  unsigned long *lengths;

/* SELECT (*) FROM list_slog WHERE fromline LIKE '%search%' OR address   */
/* LIKE '%search%' ORDER BY tai; */
/* The '*' is formatted to look like the output of the non-mysql version */
/* This requires reading the entire table, since search fields are not   */
/* indexed, but this is a rare query and time is not of the essence.     */

    if (!stralloc_copys(&line,"SELECT CONCAT(FROM_UNIXTIME(UNIX_TIMESTAMP(tai)),"
	"'-0000: ',UNIX_TIMESTAMP(tai),' ',edir,etype,' ',address,' ',"
	"fromline) FROM ")) die_nomem();
    if (!stralloc_cats(&line,info.table)) die_nomem();
    if (!stralloc_cats(&line,"_slog ")) die_nomem();
    if (*search) {	/* We can afford to wait for LIKE '%xx%' */
      if (!stralloc_cats(&line,"WHERE fromline LIKE '%")) die_nomem();
      if (!stralloc_cats(&line,search)) die_nomem();
      if (!stralloc_cats(&line,"%' OR address LIKE '%")) die_nomem();
      if (!stralloc_cats(&line,search)) die_nomem();
      if (!stralloc_cats(&line,"%'")) die_nomem();
    }	/* ordering by tai which is an index */
      if (!stralloc_cats(&line," ORDER by tai")) die_nomem();

    result = safe_select(&line);
    while ((row = mysql_fetch_row(result))) {
    if (!(lengths = mysql_fetch_lengths(result)))
	strerr_die2x(111,FATAL,mysql_error(mysql));
      if (subwrite(row[0],lengths[0]) == -1) die_write();
    }
    if (!mysql_eof(result))
	strerr_die2x(111,FATAL,mysql_error(mysql));
    mysql_free_result(result);
}

static int _subscribe(const char *dir,
		      const char *subdir,
		      const char *userhost,
		      int flagadd,
		      const char *comment,
		      const char *event,
		      int forcehash)
/* add (flagadd=1) or remove (flagadd=0) userhost from the subscr. database  */
/* dir. Comment is e.g. the subscriber from line or name. It is added to     */
/* the log. Event is the action type, e.g. "probe", "manual", etc. The       */
/* direction (sub/unsub) is inferred from flagadd. Returns 1 on success, 0   */
/* on failure. If flagmysql is set and the file "sql" is found in the        */
/* directory dir, it is parsed and a mysql db is assumed. if forcehash is    */
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

  unsigned int j;
  unsigned char ch;

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
      if (!stralloc_cats(&line,info.table)) die_nomem();
      if (!stralloc_cats(&line," WRITE")) die_nomem();
      safe_query(&line);
      if (!stralloc_copys(&line,"SELECT address FROM ")) die_nomem();
      if (!stralloc_cats(&line,info.table)) die_nomem();
      if (!stralloc_cats(&line," WHERE address='")) die_nomem();
      if (!stralloc_cat(&line,&quoted)) die_nomem();	/* addr */
      if (!stralloc_cats(&line,"'")) die_nomem();
      safe_query(&line);
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
	if (!stralloc_cats(&line,info.table)) die_nomem();
	if (!stralloc_cats(&line," (address,hash) VALUES ('"))
		die_nomem();
	if (!stralloc_cat(&line,&quoted)) die_nomem();	/* addr */
	if (!stralloc_cats(&line,"',")) die_nomem();
	if (!stralloc_cats(&line,szhash)) die_nomem();	/* hash */
	if (!stralloc_cats(&line,")")) die_nomem();
	safe_query(&line);
	if (mysql_query(mysql,"UNLOCK TABLES"))
	  strerr_die2x(111,FATAL,mysql_error(mysql));
      }
    } else {							/* unsub */
      if (!stralloc_copys(&line,"DELETE FROM ")) die_nomem();
      if (!stralloc_cats(&line,info.table)) die_nomem();
      if (!stralloc_cats(&line," WHERE address='")) die_nomem();
      if (!stralloc_cat(&line,&quoted)) die_nomem();	/* addr */
      if (forcehash >= 0) {
	if (!stralloc_cats(&line,"' AND hash=")) die_nomem();
	if (!stralloc_cats(&line,szhash)) die_nomem();
      } else {
        if (!stralloc_cats(&line,"' AND hash BETWEEN 0 AND 52"))
		die_nomem();
      }
      safe_query(&line);
      if (mysql_affected_rows(mysql) == 0)
	return 0;				/* address wasn't there*/
    }

		/* log to subscriber log */
		/* INSERT INTO t_slog (address,edir,etype,fromline) */
		/* VALUES('address',{'+'|'-'},'etype','[comment]') */

    if (!stralloc_copys(&logline,"INSERT INTO ")) die_nomem();
    if (!stralloc_cats(&logline,info.table)) die_nomem();
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
    logaddr(dir,subdir,event,addr.s,comment);	/* also log to old log */
    return 1;					/* desired effect */
}

static void _tagmsg(const char *dir,		/* db base dir */
		    unsigned long msgnum,	/* number of this message */
		    const char *seed, /* seed. NULL ok, but less entropy */
		    const char *action,	/* to make it certain the cookie differs from*/
		    /* one used for a digest */
		    char *hashout,		/* calculated hash goes here */
		    unsigned long bodysize,
		    unsigned long chunk)
/* This routine creates a cookie from num,seed and the */
/* list key and returns that cookie in hashout. The use of sender/num and */
/* first char of action is used to make cookie differ between messages,   */
/* the key is the secret list key. The cookie will be inserted into       */
/* table_cookie where table and other data is taken from dir/sql. We log  */
/* arrival of the message (done=0). */
{
  const char *ret;

    if (chunk >= 53L) chunk = 0L;	/* sanity */

	/* INSERT INTO table_cookie (msgnum,cookie) VALUES (num,cookie) */
	/* (we may have tried message before, but failed to complete, so */
	/* ER_DUP_ENTRY is ok) */
    if (!stralloc_copys(&line,"INSERT INTO ")) die_nomem();
    if (!stralloc_cats(&line,info.table)) die_nomem();
    if (!stralloc_cats(&line,"_cookie (msgnum,cookie,bodysize,chunk) VALUES ("))
		die_nomem();
    if (!stralloc_catb(&line,strnum,fmt_ulong(strnum,msgnum))) die_nomem();
    if (!stralloc_cats(&line,",'")) die_nomem();
    if (!stralloc_catb(&line,hashout,COOKIE)) die_nomem();
    if (!stralloc_cats(&line,"',")) die_nomem();
    if (!stralloc_catb(&line,strnum,fmt_ulong(strnum,bodysize)))
		die_nomem();
    if (!stralloc_cats(&line,",")) die_nomem();
    if (!stralloc_catb(&line,strnum,fmt_ulong(strnum,chunk))) die_nomem();
    if (!stralloc_cats(&line,")")) die_nomem();
    if (mysql_real_query(mysql,line.s,line.len) != 0)
      if (mysql_errno(mysql) != ER_DUP_ENTRY)	/* ignore dups */
        strerr_die2x(111,FATAL,mysql_error(mysql)); /* cookie query */

    if (! (ret = logmsg(dir,msgnum,0L,0L,1))) return;	/* log done=1*/
    if (*ret) strerr_die2x(111,FATAL,ret);
}

struct sub_plugin sub_plugin = {
  SUB_PLUGIN_VERSION,
  _checktag,
  _closesub,
  _issub,
  _logmsg,
  _opensub,
  _putsubs,
  _searchlog,
  _subscribe,
  _tagmsg,
};
