#include "byte.h"
#include "case.h"
#include "cookie.h"
#include "date822fmt.h"
#include "datetime.h"
#include "die.h"
#include "error.h"
#include "messages.h"
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
#include "str.h"
#include "stralloc.h"
#include "strerr.h"
#include "sub_sql.h"
#include "sub_std.h"
#include "subhash.h"
#include "subdb.h"
#include "substdio.h"
#include "uint32.h"
#include "wrap.h"
#include <sys/types.h>
#include <mysql.h>
#include <mysqld_error.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>

static char strnum[FMT_ULONG];
static stralloc addr = {0};
static stralloc domain = {0};
static stralloc lcaddr = {0};
static stralloc line = {0};
static stralloc logline = {0};
static stralloc quoted = {0};

static void die_write(void)
{
  strerr_die2sys(111,FATAL,MSG(ERR_WRITE_STDOUT));
}

static int stralloc_cat_table(stralloc *s,
			      const struct subdbinfo *info,
			      const char *table)
{
  if (!stralloc_cats(s,info->base_table)) return 0;
  if (table) {
    if (!stralloc_append(s,"_")) return 0;
    if (!stralloc_cats(s,table)) return 0;
  }
  return 1;
}

/* close connection to SQL server, if open */
static void _closesub(struct subdbinfo *info)
{
  if ((MYSQL*)info->conn)
    mysql_close((MYSQL*)info->conn);
  info->conn = 0;					/* destroy pointer */
}

/* open connection to the SQL server, if it isn't already open. */
static const char *_opensub(struct subdbinfo *info)
{
  if (!(MYSQL*)info->conn) {
    if (!(info->conn = mysql_init((MYSQL *) 0)))
	 die_nomem();					/* init */
    if (!(mysql_real_connect((MYSQL*)info->conn,info->host,info->user,info->pw,
			     info->db,info->port,0,0)))
		return mysql_error((MYSQL*)info->conn);
  }
  return (char *) 0;
}

static void safe_query(struct subdbinfo *info,const stralloc* query)
{
  if (mysql_real_query((MYSQL*)info->conn,query->s,query->len))
    strerr_die2x(111,FATAL,mysql_error((MYSQL*)info->conn));
}

static MYSQL_RES *safe_select(struct subdbinfo *info,const stralloc* query)
{
  MYSQL_RES *result;
  safe_query(info,query);
  if ((result = mysql_use_result((MYSQL*)info->conn)) == 0)
    strerr_die2x(111,FATAL,mysql_error((MYSQL*)info->conn));
  return result;
}

/* Checks the hash against the cookie table. If it matches, returns NULL,
 * else returns "". If error, returns error string. */
static const char *_checktag (struct subdbinfo *info,
			      unsigned long num,	/* message number */
			      unsigned long listno,	/* bottom of range => slave */
			      const char *action,
			      const char *seed,
			      const char *hash)		/* cookie */
{
  MYSQL_RES *result;
  MYSQL_ROW row;

/* SELECT msgnum FROM table_cookie WHERE msgnum=num and cookie='hash' */
/* succeeds only is everything correct. 'hash' is quoted since it is  */
/*  potentially hostile. */
    if (listno) {			/* only for slaves */
      if (!stralloc_copys(&line,"SELECT listno FROM ")) die_nomem();
      if (!stralloc_cats(&line,info->base_table)) die_nomem();
      if (!stralloc_cats(&line,"_mlog WHERE listno=")) die_nomem();
      if (!stralloc_catb(&line,strnum,fmt_ulong(strnum,listno)))
	die_nomem();
      if (!stralloc_cats(&line," AND msgnum=")) die_nomem();
      if (!stralloc_catb(&line,strnum,fmt_ulong(strnum,num))) die_nomem();
      if (!stralloc_cats(&line," AND done > 3")) die_nomem();
      if (mysql_real_query((MYSQL*)info->conn,line.s,line.len) != 0)
	return mysql_error((MYSQL*)info->conn);			/* query */
      if (!(result = mysql_use_result((MYSQL*)info->conn)))		/* use result */
	return mysql_error((MYSQL*)info->conn);
      if ((row = mysql_fetch_row(result)))
	return "";					/*already done */
      else						/* no result */
        if (!mysql_eof(result))
	  return mysql_error((MYSQL*)info->conn);
      mysql_free_result(result);			/* free res */
    }

    if (!stralloc_copys(&line,"SELECT msgnum FROM ")) die_nomem();
    if (!stralloc_cats(&line,info->base_table)) die_nomem();
    if (!stralloc_cats(&line,"_cookie WHERE msgnum=")) die_nomem();
    if (!stralloc_catb(&line,strnum,fmt_ulong(strnum,num))) die_nomem();
    if (!stralloc_cats(&line," and cookie='")) die_nomem();
    if (!stralloc_ready(&quoted,COOKIE * 2 + 1)) die_nomem();
    quoted.len = mysql_escape_string(quoted.s,hash,COOKIE);
    if (!stralloc_cat(&line,&quoted)) die_nomem();
    if (!stralloc_cats(&line,"'")) die_nomem();

    if (mysql_real_query((MYSQL*)info->conn,line.s,line.len) != 0)	/* select */
	return mysql_error((MYSQL*)info->conn);
    if (!(result = mysql_use_result((MYSQL*)info->conn)))
	return mysql_error((MYSQL*)info->conn);
    if (!mysql_fetch_row(result)) {
    if (!mysql_eof(result))		/* some error occurred */
	return mysql_error((MYSQL*)info->conn);
      mysql_free_result(result);	/* eof => query ok, but null result*/
      return "";			/* not parent => perm error */
    }
    mysql_free_result(result);		/* success! cookie matches */
    return (char *)0;
    (void)action;
    (void)seed;
}

/* Creates an entry for message num and the list listno and code "done".
 * Returns NULL on success, and the error string on error. */
static const char *_logmsg(struct subdbinfo *info,
			   unsigned long num,
			   unsigned long listno,
			   unsigned long subs,
			   int done)
{
  if (!stralloc_copys(&logline,"INSERT INTO ")) die_nomem();
  if (!stralloc_cats(&logline,info->base_table)) die_nomem();
  if (!stralloc_cats(&logline,"_mlog (msgnum,listno,subs,done) VALUES ("))
	die_nomem();
  if (!stralloc_catb(&logline,strnum,fmt_ulong(strnum,num))) die_nomem();
  if (!stralloc_cats(&logline,",")) die_nomem();
  if (!stralloc_catb(&logline,strnum,fmt_ulong(strnum,listno)))
	die_nomem();
  if (!stralloc_cats(&logline,",")) die_nomem();
  if (!stralloc_catb(&logline,strnum,fmt_ulong(strnum,subs))) die_nomem();
  if (!stralloc_cats(&logline,",")) die_nomem();
  if (done < 0) {
    done = - done;
    if (!stralloc_append(&logline,"-")) die_nomem();
  }
  if (!stralloc_catb(&logline,strnum,fmt_uint(strnum,done))) die_nomem();
  if (!stralloc_append(&logline,")")) die_nomem();

  if (mysql_real_query((MYSQL*)info->conn,logline.s,logline.len))	/* log query */
    if (mysql_errno((MYSQL*)info->conn) != ER_DUP_ENTRY)	/* ignore dups */
	return mysql_error((MYSQL*)info->conn);
  return 0;
}

/* Outputs all addresses in the table through subwrite. subwrite must be
 * a function returning >=0 on success, -1 on error, and taking
 * arguments (char* string, unsigned int length). It will be called once
 * per address and should take care of newline or whatever needed for
 * the output form. */
static unsigned long _putsubs(struct subdbinfo *info,
			      const char *table,
			      unsigned long hash_lo,
			      unsigned long hash_hi,
			      int subwrite())		/* write function. */
{
  MYSQL_RES *result;
  MYSQL_ROW row;
  unsigned long *lengths;

  unsigned long no = 0L;

						/* main query */
    if (!stralloc_copys(&line,"SELECT address FROM "))
		die_nomem();
    if (!stralloc_cat_table(&line,info,table)) die_nomem();
    if (!stralloc_cats(&line," WHERE hash BETWEEN ")) die_nomem();
    if (!stralloc_catb(&line,strnum,fmt_ulong(strnum,hash_lo)))
		die_nomem();
    if (!stralloc_cats(&line," AND ")) die_nomem();
    if (!stralloc_catb(&line,strnum,fmt_ulong(strnum,hash_hi)))
		die_nomem();
    result = safe_select(info,&line);
    no = 0;
    while ((row = mysql_fetch_row(result))) {
	/* this is safe even if someone messes with the address field def */
    if (!(lengths = mysql_fetch_lengths(result)))
	strerr_die2x(111,FATAL,mysql_error((MYSQL*)info->conn));
      if (subwrite(row[0],lengths[0]) == -1) die_write();
      no++;					/* count for list-list fxn */
    }
    if (!mysql_eof(result))
	strerr_die2x(111,FATAL,mysql_error((MYSQL*)info->conn));
    mysql_free_result(result);
    return no;
}

/* Searches the subscriber log and outputs via subwrite(s,len) any entry
 * that matches search. A '_' is search is a wildcard. Any other
 * non-alphanum/'.' char is replaced by a '_'. */
static void _searchlog(struct subdbinfo *info,
		       const char *table,
		       char *search,		/* search string */
		       int subwrite())		/* output fxn */
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
    if (!stralloc_cat_table(&line,info,table)) die_nomem();
    if (!stralloc_cats(&line,"_slog ")) die_nomem();
    if (*search) {	/* We can afford to wait for LIKE '%xx%' */
      if (!stralloc_cats(&line,"WHERE fromline LIKE '%")) die_nomem();
      if (!stralloc_cats(&line,search)) die_nomem();
      if (!stralloc_cats(&line,"%' OR address LIKE '%")) die_nomem();
      if (!stralloc_cats(&line,search)) die_nomem();
      if (!stralloc_cats(&line,"%'")) die_nomem();
    }	/* ordering by tai which is an index */
      if (!stralloc_cats(&line," ORDER by tai")) die_nomem();

    result = safe_select(info,&line);
    while ((row = mysql_fetch_row(result))) {
    if (!(lengths = mysql_fetch_lengths(result)))
	strerr_die2x(111,FATAL,mysql_error((MYSQL*)info->conn));
      if (subwrite(row[0],lengths[0]) == -1) die_write();
    }
    if (!mysql_eof(result))
	strerr_die2x(111,FATAL,mysql_error((MYSQL*)info->conn));
    mysql_free_result(result);
}

/* Add (flagadd=1) or remove (flagadd=0) userhost from the subscriber
 * database table. Comment is e.g. the subscriber from line or name. It
 * is added to the log. Event is the action type, e.g. "probe",
 * "manual", etc. The direction (sub/unsub) is inferred from
 * flagadd. Returns 1 on success, 0 on failure. If forcehash is >=0 it
 * is used in place of the calculated hash. This makes it possible to
 * add addresses with a hash that does not exist. forcehash has to be
 * 0..99.  For unsubscribes, the address is only removed if forcehash
 * matches the actual hash. This way, ezmlm-manage can be prevented from
 * touching certain addresses that can only be removed by
 * ezmlm-unsub. Usually, this would be used for sublist addresses (to
 * avoid removal) and sublist aliases (to prevent users from subscribing
 * them (although the cookie mechanism would prevent the resulting
 * duplicate message from being distributed. */
static int _subscribe(struct subdbinfo *info,
		      const char *table,
		      const char *userhost,
		      int flagadd,
		      const char *comment,
		      const char *event,
		      int forcehash)
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
      strerr_die2x(100,FATAL,MSG(ERR_ADDR_LONG));
    j = byte_rchr(addr.s,addr.len,'@');
    if (j == addr.len)
      strerr_die2x(100,FATAL,MSG(ERR_ADDR_AT));
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
      if (!stralloc_cat_table(&line,info,table)) die_nomem();
      if (!stralloc_cats(&line," WRITE")) die_nomem();
      safe_query(info,&line);
      if (!stralloc_copys(&line,"SELECT address FROM ")) die_nomem();
      if (!stralloc_cat_table(&line,info,table)) die_nomem();
      if (!stralloc_cats(&line," WHERE address='")) die_nomem();
      if (!stralloc_cat(&line,&quoted)) die_nomem();	/* addr */
      if (!stralloc_cats(&line,"'")) die_nomem();
      safe_query(info,&line);
      if (!(result = mysql_use_result((MYSQL*)info->conn)))
	strerr_die2x(111,FATAL,mysql_error((MYSQL*)info->conn));
      if ((row = mysql_fetch_row(result))) {			/* there */
	while (mysql_fetch_row(result));			/* use'm up */
	mysql_free_result(result);
	if (mysql_query((MYSQL*)info->conn,"UNLOCK TABLES"))
	  strerr_die2x(111,FATAL,mysql_error((MYSQL*)info->conn));
        return 0;						/* there */
      } else {							/* not there */
	mysql_free_result(result);
	if (mysql_errno((MYSQL*)info->conn))			/* or ERROR */
	  strerr_die2x(111,FATAL,mysql_error((MYSQL*)info->conn));
	if (!stralloc_copys(&line,"INSERT INTO ")) die_nomem();
	if (!stralloc_cat_table(&line,info,table)) die_nomem();
	if (!stralloc_cats(&line," (address,hash) VALUES ('"))
		die_nomem();
	if (!stralloc_cat(&line,&quoted)) die_nomem();	/* addr */
	if (!stralloc_cats(&line,"',")) die_nomem();
	if (!stralloc_cats(&line,szhash)) die_nomem();	/* hash */
	if (!stralloc_cats(&line,")")) die_nomem();
	safe_query(info,&line);
	if (mysql_query((MYSQL*)info->conn,"UNLOCK TABLES"))
	  strerr_die2x(111,FATAL,mysql_error((MYSQL*)info->conn));
      }
    } else {							/* unsub */
      if (!stralloc_copys(&line,"DELETE FROM ")) die_nomem();
      if (!stralloc_cat_table(&line,info,table)) die_nomem();
      if (!stralloc_cats(&line," WHERE address='")) die_nomem();
      if (!stralloc_cat(&line,&quoted)) die_nomem();	/* addr */
      if (forcehash >= 0) {
	if (!stralloc_cats(&line,"' AND hash=")) die_nomem();
	if (!stralloc_cats(&line,szhash)) die_nomem();
      } else {
        if (!stralloc_cats(&line,"' AND hash BETWEEN 0 AND 52"))
		die_nomem();
      }
      safe_query(info,&line);
      if (mysql_affected_rows((MYSQL*)info->conn) == 0)
	return 0;				/* address wasn't there*/
    }

		/* log to subscriber log */
		/* INSERT INTO t_slog (address,edir,etype,fromline) */
		/* VALUES('address',{'+'|'-'},'etype','[comment]') */

    if (!stralloc_copys(&logline,"INSERT INTO ")) die_nomem();
    if (!stralloc_cat_table(&logline,info,table)) die_nomem();
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

    if (mysql_real_query((MYSQL*)info->conn,logline.s,logline.len))
		;				/* log (ignore errors) */
    if (!stralloc_0(&addr))
		;				/* ignore errors */
    logaddr(table,event,addr.s,comment);	/* also log to old log */
    return 1;					/* desired effect */
}

/* This routine inserts the cookie into table_cookie. We log arrival of
 * the message (done=0). */
static void _tagmsg(struct subdbinfo *info,
		    unsigned long msgnum,	/* number of this message */
		    const char *hashout,	/* previously calculated hash */
		    unsigned long bodysize,
		    unsigned long chunk)
{
  const char *ret;

    if (chunk >= 53L) chunk = 0L;	/* sanity */

	/* INSERT INTO table_cookie (msgnum,cookie) VALUES (num,cookie) */
	/* (we may have tried message before, but failed to complete, so */
	/* ER_DUP_ENTRY is ok) */
    if (!stralloc_copys(&line,"INSERT INTO ")) die_nomem();
    if (!stralloc_cats(&line,info->base_table)) die_nomem();
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
    if (mysql_real_query((MYSQL*)info->conn,line.s,line.len) != 0)
      if (mysql_errno((MYSQL*)info->conn) != ER_DUP_ENTRY)	/* ignore dups */
        strerr_die2x(111,FATAL,mysql_error((MYSQL*)info->conn)); /* cookie query */

    if (! (ret = logmsg(msgnum,0L,0L,1))) return;	/* log done=1*/
    if (*ret) strerr_die2x(111,FATAL,ret);
}

static void die_sqlerror(struct subdbinfo *info)
{
  strerr_die2x(111,FATAL,mysql_error((MYSQL*)info->conn));
}

void *sql_select(struct subdbinfo *info,
		 struct stralloc *q,
		 unsigned int nparams,
		 struct stralloc *params)
{
  MYSQL_STMT *stmt;
  MYSQL_BIND bind[nparams];
  unsigned int i;

  if ((stmt = mysql_stmt_init((MYSQL*)info->conn)) == 0)
    die_sqlerror(info);
  if (mysql_stmt_prepare(stmt,q->s,q->len) != 0)
    die_sqlerror(info);
  byte_zero((char*)bind,sizeof bind);
  for (i = 0; i < nparams; ++i) {
    bind[i].buffer_type = MYSQL_TYPE_STRING;
    bind[i].buffer = params[i].s;
    bind[i].buffer_length = params[i].len;
  }
  if (mysql_stmt_bind_param(stmt,bind) != 0)
    die_sqlerror(info);
  if (mysql_stmt_execute(stmt) != 0)
    die_sqlerror(info);
  return stmt;
}

int sql_fetch_row(struct subdbinfo *info,
		  void *result,
		  unsigned int ncolumns,
		  struct stralloc *columns)
{
  MYSQL_BIND bind[ncolumns];
  unsigned long lengths[ncolumns];
  unsigned int i;

  byte_zero((char*)bind,sizeof bind);
  byte_zero((char*)lengths,sizeof lengths);
  for (i = 0; i < ncolumns; ++i) {
    bind[i].buffer_type = MYSQL_TYPE_BLOB;
    bind[i].buffer = 0;
    bind[i].buffer_length = 0;
    bind[i].length = &lengths[i];
  }

  if (mysql_stmt_bind_result((MYSQL_STMT*)result,bind) != 0)
    die_sqlerror(info);
  switch (mysql_stmt_fetch((MYSQL_STMT*)result)) {
  case MYSQL_DATA_TRUNCATED:	/* expect this since buffer_length == 0 */
    break;
  case MYSQL_NO_DATA:
    return 0;
  default:
    die_sqlerror(info);
  }

  for (i = 0; i < ncolumns; ++i) {
    if (!stralloc_ready(&columns[i],lengths[i])) die_nomem();
    bind[i].buffer = columns[i].s;
    bind[i].buffer_length = lengths[i];
    mysql_stmt_fetch_column((MYSQL_STMT*)result,&bind[i],i,0);
    columns[i].len = lengths[i];
  }

  return 1;
}

void sql_free_result(struct subdbinfo *info,
		     void *result)
{
  if (mysql_stmt_close((MYSQL_STMT*)result) != 0)
    die_sqlerror(info);
  (void)info;
}

const char *sql_create_table(struct subdbinfo *info,
			     const char *defn)
{
  if (mysql_real_query((MYSQL*)info->conn,defn,str_len(defn)) != 0)
    if (mysql_errno((MYSQL*)info->conn) != ER_TABLE_EXISTS_ERROR)
      return mysql_error((MYSQL*)info->conn);
  return 0;
}

/* Address table */
/* Need varchar. Domain = 3 chars => fixed length, as opposed to varchar
 * Always select on domain and hash, so that one index should do primary
 * key(address) is very inefficient for MySQL.  MySQL tables do not need
 * a primary key. Other RDBMS require one. For the log tables, just add
 * an INT AUTO_INCREMENT. For the address table, do that or use address
 * as a primary key. */
const char sql_sub_table_defn[] =
  "  hash    TINYINT UNSIGNED NOT NULL,"
  "  address VARCHAR(255) NOT NULL,"
  "  INDEX h (hash),"
  "  INDEX a (address(12))";

/* Subscription log table. No addr idx to make insertion fast, since
 * that is almost the only thing we do with this table */
const char sql_slog_table_defn[] =
  "  tai		TIMESTAMP,"
  "  address	VARCHAR(255) NOT NULL,"
  "  fromline	VARCHAR(255) NOT NULL,"
  "  edir		CHAR(1) NOT NULL,"
  "  etype	CHAR(1) NOT NULL,"
  "  INDEX (tai)";

/* main list inserts a cookie here. Sublists check it */
const char sql_cookie_table_defn[] =
  "  msgnum	INTEGER UNSIGNED NOT NULL,"
  "  tai	TIMESTAMP NOT NULL,"
  "  cookie	CHAR(20) NOT NULL,"
  "  chunk	TINYINT UNSIGNED NOT NULL DEFAULT 0,"
  "  bodysize	INTEGER UNSIGNED NOT NULL DEFAULT 0,"
  "  PRIMARY KEY (msgnum)";

/* main and sublist log here when the message is done done=0 for
 * arrived, done=4 for sent, 5 for receit.  tai reflects last change */
const char sql_mlog_table_defn[] =
  "msgnum	INTEGER UNSIGNED NOT NULL,"
  "listno	INTEGER UNSIGNED NOT NULL,"
  "tai		TIMESTAMP,"
  "subs		INTEGER UNSIGNED NOT NULL DEFAULT 0,"
  "done		TINYINT NOT NULL DEFAULT 0,"
  "PRIMARY KEY listmsg (listno,msgnum,done)";

/* Definition of WHERE clause for selecting addresses in issub */
const char sql_issub_where_defn[] = "address LIKE ?";

int sql_table_exists(struct subdbinfo *info,
		     const char *name)
{
  MYSQL_RES *result;

  if (!stralloc_copys(&line,"SELECT 0 FROM ")) die_nomem();
  if (!stralloc_cats(&line,name)) die_nomem();
  if (!stralloc_cats(&line," LIMIT 1")) die_nomem();
  if (!stralloc_0(&line)) die_nomem();
  if (mysql_real_query((MYSQL*)info->conn,line.s,line.len) == 0) {
    if ((result = mysql_use_result((MYSQL*)info->conn)) != 0)
      mysql_free_result(result);
    return 1;
  }
  return (mysql_errno((MYSQL*)info->conn) == ER_BAD_TABLE_ERROR) ? 0 : -1;
}

const char *sql_drop_table(struct subdbinfo *info,
			   const char *name)
{
  if (!stralloc_copys(&line,"DROP TABLE ")) die_nomem();
  if (!stralloc_cats(&line,name)) die_nomem();
  if (mysql_real_query((MYSQL*)info->conn,line.s,line.len) != 0)
    if (mysql_errno((MYSQL*)info->conn) != ER_BAD_TABLE_ERROR)
      return mysql_error((MYSQL*)info->conn);
  return 0;
}

struct sub_plugin sub_plugin = {
  SUB_PLUGIN_VERSION,
  _checktag,
  _closesub,
  sub_sql_issub,
  _logmsg,
  sub_sql_mktab,
  _opensub,
  _putsubs,
  sub_sql_rmtab,
  _searchlog,
  _subscribe,
  _tagmsg,
};
