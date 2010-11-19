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
#include "now.h"
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
#include <sqlite3.h>
#include <unistd.h>

static char strnum[FMT_ULONG];
static stralloc addr = {0};
static stralloc domain = {0};
static stralloc lcaddr = {0};
static stralloc line = {0};
static stralloc logline = {0};
static stralloc quoted = {0};

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

static sqlite3_stmt *_sqlquery(const struct subdbinfo *info, const stralloc *str)
{
	sqlite3_stmt *stmt;

	if (sqlite3_prepare_v2((sqlite3*)info->conn, str->s, str->len, &stmt, NULL) != SQLITE_OK)
		return NULL;

	return stmt;
}

/* close connection to SQL server, if open */
static void _closesub(struct subdbinfo *info)
{
  if ((sqlite3*)info->conn)
    sqlite3_close((sqlite3*)info->conn);
  info->conn = 0;					/* destroy pointer */
}

/* open connection to the SQL server, if it isn't already open. */
static const char *_opensub(struct subdbinfo *info)
{
  if (!(sqlite3*)info->conn) {
    if (!stralloc_copys(&line,info->db)) die_nomem();
    if (!stralloc_cats(&line,".db")) die_nomem();
    if (!stralloc_0(&line)) die_nomem();
    if (sqlite3_open_v2(line.s, (sqlite3**)&info->conn, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL) != SQLITE_OK)
	return sqlite3_errmsg((sqlite3*)info->conn); /* init */
  }
  return (char *) 0;
}

/* Creates an entry for message num and the list listno and code "done".
 * Returns NULL on success, and the error string on error. */
static const char *_logmsg(struct subdbinfo *info,
			   unsigned long num,
			   unsigned long listno,
			   unsigned long subs,
			   int done)
{
  sqlite3_stmt *stmt;
  int res;

  if (!stralloc_copys(&logline,"INSERT INTO ")) die_nomem();
  if (!stralloc_cats(&logline,info->base_table)) die_nomem();
  if (!stralloc_cats(&logline,"_mlog (msgnum,listno,tai,subs,done) VALUES ("))
	die_nomem();
  if (!stralloc_catb(&logline,strnum,fmt_ulong(strnum,num))) die_nomem();
  if (!stralloc_cats(&logline,",")) die_nomem();
  if (!stralloc_catb(&logline,strnum,fmt_ulong(strnum,listno)))
	die_nomem();
  if (!stralloc_cats(&line,",")) die_nomem();
  if (!stralloc_catb(&line,strnum,fmt_ulong(strnum,now()))) die_nomem();
  if (!stralloc_cats(&line,",'")) die_nomem();
  if (!stralloc_catb(&logline,strnum,fmt_ulong(strnum,subs))) die_nomem();
  if (!stralloc_cats(&logline,",")) die_nomem();
  if (done < 0) {
    done = - done;
    if (!stralloc_append(&logline,"-")) die_nomem();
  }
  if (!stralloc_catb(&logline,strnum,fmt_uint(strnum,done))) die_nomem();
  if (!stralloc_append(&logline,")")) die_nomem();
  if (!stralloc_0(&logline)) die_nomem();

  if ((stmt = _sqlquery(info, &logline)) == NULL)	/* log query */
	  return sqlite3_errmsg((sqlite3*)info->conn);

  res = sqlite3_step(stmt);
  sqlite3_finalize(stmt);

  if (res != SQLITE_DONE)
  {
	  if (res != SQLITE_CONSTRAINT)		/* ignore dups */
		  return sqlite3_errmsg((sqlite3*)info->conn);
  }

  return 0;
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
  sqlite3_stmt *stmt;
  char *cpat;
  char szhash[3] = "00";
  int res;

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
    if (!stralloc_copy(&quoted, &addr)) die_nomem();
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
      if (!stralloc_copys(&line,"SELECT address FROM ")) die_nomem();
      if (!stralloc_cat_table(&line,info,table)) die_nomem();
      if (!stralloc_cats(&line," WHERE address LIKE '")) die_nomem();
      if (!stralloc_cat(&line,&quoted)) die_nomem();	/* addr */
      if (!stralloc_cats(&line,"'")) die_nomem();
	  if (!stralloc_0(&line)) die_nomem();

	  if ((stmt = _sqlquery(info, &line)) == NULL)
		  strerr_die2x(111,FATAL,sqlite3_errmsg((sqlite3*)info->conn));

	  res = sqlite3_step(stmt);
	  sqlite3_finalize(stmt);

	  if (res == SQLITE_ROW)
		  return 0;						/* there */
	  else if (res != SQLITE_DONE)
	  {
		  strerr_die2x(111,FATAL,sqlite3_errmsg((sqlite3*)info->conn));
      } else {							/* not there */
	if (!stralloc_copys(&line,"INSERT INTO ")) die_nomem();
	if (!stralloc_cat_table(&line,info,table)) die_nomem();
	if (!stralloc_cats(&line," (address,hash) VALUES ('"))
		die_nomem();
	if (!stralloc_cat(&line,&quoted)) die_nomem();	/* addr */
	if (!stralloc_cats(&line,"',")) die_nomem();
	if (!stralloc_cats(&line,szhash)) die_nomem();	/* hash */
	if (!stralloc_cats(&line,")")) die_nomem();
	if (!stralloc_0(&line)) die_nomem();

	if ((stmt = _sqlquery(info, &line)) == NULL)
		strerr_die2x(111,FATAL,sqlite3_errmsg((sqlite3*)info->conn));

	if (sqlite3_step(stmt) != SQLITE_DONE)
		strerr_die2x(111,FATAL,sqlite3_errmsg((sqlite3*)info->conn));

	sqlite3_finalize(stmt);
	  }
    } else {							/* unsub */
      if (!stralloc_copys(&line,"DELETE FROM ")) die_nomem();
      if (!stralloc_cat_table(&line,info,table)) die_nomem();
      if (!stralloc_cats(&line," WHERE address LIKE '")) die_nomem();
      if (!stralloc_cat(&line,&quoted)) die_nomem();	/* addr */
      if (forcehash >= 0) {
	if (!stralloc_cats(&line,"' AND hash=")) die_nomem();
	if (!stralloc_cats(&line,szhash)) die_nomem();
      } else {
        if (!stralloc_cats(&line,"' AND hash BETWEEN 0 AND 52"))
		die_nomem();
      }

	  if (!stralloc_0(&line)) die_nomem();

	  if ((stmt = _sqlquery(info, &line)) == NULL)
		  strerr_die2x(111,FATAL,sqlite3_errmsg((sqlite3*)info->conn));

	  if (sqlite3_step(stmt) != SQLITE_DONE)
		  strerr_die2x(111,FATAL,sqlite3_errmsg((sqlite3*)info->conn));

	  sqlite3_finalize(stmt);

      if (sqlite3_changes((sqlite3*)info->conn) == 0)
	return 0;				/* address wasn't there*/
    }

		/* log to subscriber log */
		/* INSERT INTO t_slog (address,edir,etype,fromline) */
		/* VALUES('address',{'+'|'-'},'etype','[comment]') */

    if (!stralloc_copys(&logline,"INSERT INTO ")) die_nomem();
    if (!stralloc_cat_table(&logline,info,table)) die_nomem();
    if (!stralloc_cats(&logline,
		       "_slog (tai,address,edir,etype,fromline) VALUES ("))
      die_nomem();
    if (!stralloc_catb(&logline,strnum,fmt_ulong(strnum,now()))) die_nomem();
    if (!stralloc_cats(&logline,",'")) die_nomem();
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
	  if (!stralloc_copys(&quoted, comment)) die_nomem();	/* from */
	  if (!stralloc_cat(&logline,&quoted)) die_nomem();
    }
    if (!stralloc_cats(&logline,"')")) die_nomem();
	if (!stralloc_0(&logline)) die_nomem();

	if ((stmt = _sqlquery(info, &logline)) != NULL)
	{
		sqlite3_step(stmt);			/* log (ignore errors) */
		sqlite3_finalize(stmt);
	}

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
  sqlite3_stmt *stmt;
  const char *ret;
  int res;

    if (chunk >= 53L) chunk = 0L;	/* sanity */

	/* INSERT INTO table_cookie (msgnum,cookie) VALUES (num,cookie) */
	/* (we may have tried message before, but failed to complete, so */
	/* ER_DUP_ENTRY is ok) */
    if (!stralloc_copys(&line,"INSERT INTO ")) die_nomem();
    if (!stralloc_cats(&line,info->base_table)) die_nomem();
    if (!stralloc_cats(&line,"_cookie (msgnum,tai,cookie,bodysize,chunk) VALUES ("))
		die_nomem();
    if (!stralloc_catb(&line,strnum,fmt_ulong(strnum,msgnum))) die_nomem();
    if (!stralloc_cats(&line,",")) die_nomem();
    if (!stralloc_catb(&line,strnum,fmt_ulong(strnum,now()))) die_nomem();
    if (!stralloc_cats(&line,",'")) die_nomem();
    if (!stralloc_catb(&line,hashout,COOKIE)) die_nomem();
    if (!stralloc_cats(&line,"',")) die_nomem();
    if (!stralloc_catb(&line,strnum,fmt_ulong(strnum,bodysize)))
		die_nomem();
    if (!stralloc_cats(&line,",")) die_nomem();
    if (!stralloc_catb(&line,strnum,fmt_ulong(strnum,chunk))) die_nomem();
    if (!stralloc_cats(&line,")")) die_nomem();
	if (!stralloc_0(&line)) die_nomem();

	if ((stmt = _sqlquery(info, &line)) == NULL)
		strerr_die2x(111,FATAL,sqlite3_errmsg((sqlite3*)info->conn));

	res = sqlite3_step(stmt);
	sqlite3_finalize(stmt);

	if (res != SQLITE_DONE)
	{
		if (res != SQLITE_CONSTRAINT)		/* ignore dups */
			strerr_die2x(111,FATAL,sqlite3_errmsg((sqlite3*)info->conn));	/* cookie query */
	}

    if (! (ret = logmsg(msgnum,0L,0L,1))) return;	/* log done=1*/
    if (*ret) strerr_die2x(111,FATAL,ret);
}

static void die_sqlerror(struct subdbinfo *info)
{
  strerr_die2x(111,FATAL,sqlite3_errmsg((sqlite3*)info->conn));
}
  
void *sql_select(struct subdbinfo *info,
		 struct stralloc *q,
		 unsigned int nparams,
		 struct stralloc *params)
{
  unsigned int i;
  sqlite3_stmt *stmt;
  if (sqlite3_prepare_v2((sqlite3*)info->conn,q->s,q->len,&stmt,NULL) != SQLITE_OK)
    die_sqlerror(info);
  for (i = 0; i < nparams; ++i) {
    if (sqlite3_bind_text(stmt,i+1,params[i].s,params[i].len,SQLITE_STATIC) != SQLITE_OK)
      die_sqlerror(info);
  }
  return stmt;
}

int sql_fetch_row(struct subdbinfo *info,
		  void *result,
		  unsigned int ncolumns,
		  struct stralloc *columns)
{
  unsigned int i;
  sqlite3_stmt *stmt = result;

  switch (sqlite3_step(stmt)) {
  case SQLITE_DONE:
    return 0;
  case SQLITE_ROW:
    break;
  default:
    die_sqlerror(info);
  }

  for (i = 0; i < ncolumns; ++i) {
    if (!stralloc_copyb(&columns[i],(char*)sqlite3_column_text(stmt,i),sqlite3_column_bytes(stmt,i)))
      die_nomem();
  }
  return 1;
}

extern void sql_free_result(struct subdbinfo *info,
			    void *result)
{
  sqlite3_finalize((sqlite3_stmt*)result);
  (void)info;
}

int sql_table_exists(struct subdbinfo *info,
		     const char *name)
{
  sqlite3_stmt *stmt;
  int res;

  if (!stralloc_copys(&line,"SELECT name FROM sqlite_master WHERE name='")) return -1;
  if (!stralloc_cats(&line,name)) return -1;
  if (!stralloc_append(&line,"'")) return -1;
  if (!stralloc_0(&line)) return -1;

  if ((stmt = _sqlquery(info, &line)) == NULL)
	  return -1;
  res = sqlite3_step(stmt);
  sqlite3_finalize(stmt);
  if (res == SQLITE_ROW)
	  return 1;
  else if (res != SQLITE_DONE)
	  return -1;
  return 0;
}

const char *sql_create_table(struct subdbinfo *info,
			     const char *definition)
{
  sqlite3_stmt *stmt;
  int res;

  if (!stralloc_copys(&line,definition)) die_nomem();
  if (!stralloc_0(&line)) die_nomem();

  if ((stmt = _sqlquery(info, &line)) == NULL)
	  return sqlite3_errmsg((sqlite3*)info->conn);

  res = sqlite3_step(stmt);
  sqlite3_finalize(stmt);

  if (res != SQLITE_DONE)
	  return sqlite3_errmsg((sqlite3*)info->conn);
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
  "  hash    INT4 NOT NULL,"
  "  address VARCHAR(255) NOT NULL PRIMARY KEY";

/* Subscription log table. No addr idx to make insertion fast, since
 * that is almost the only thing we do with this table */
const char sql_slog_table_defn[] =
  "  tai	INTEGER,"
  "  address	VARCHAR(255) NOT NULL,"
  "  fromline	VARCHAR(255) NOT NULL,"
  "  edir	CHAR NOT NULL,"
  "  etype	CHAR NOT NULL";

/* main list inserts a cookie here. Sublists check it */
const char sql_cookie_table_defn[] =
  "  msgnum	INT4 NOT NULL PRIMARY KEY,"
  "  tai	INTEGER NOT NULL,"
  "  cookie	CHAR(20) NOT NULL,"
  "  chunk	INT4 NOT NULL DEFAULT 0,"
  "  bodysize	INT4 NOT NULL DEFAULT 0";

/* main and sublist log here when the message is done done=0 for
 * arrived, done=4 for sent, 5 for receit.  tai reflects last change */
const char sql_mlog_table_defn[] =
  "msgnum	INT4 NOT NULL,"
  "listno	INT4 NOT NULL,"
  "tai		TIMESTAMP,"
  "subs		INT4 NOT NULL DEFAULT 0,"
  "done		INT4 NOT NULL DEFAULT 0,"
  "PRIMARY KEY (listno,msgnum,done)";

/* Definition of WHERE clauses for checktag */
const char sql_checktag_listno_where_defn[] = "listno=? AND msgnum=? AND done > 3";
const char sql_checktag_msgnum_where_defn[] = "msgnum=? AND cookie=?";

/* Definition of WHERE clause for selecting addresses in issub */
const char sql_issub_where_defn[] = "address LIKE ?";

/* Definition of WHERE clause for selecting addresses in putsubs */
const char sql_putsubs_where_defn[] = "hash BETWEEN ? AND ?";

/* Definition of clauses for searchlog query */
const char sql_searchlog_select_defn[] = "tai, edir||etype||' '||address||' '||fromline";
const char sql_searchlog_where_defn[] = "fromline LIKE concat('%',?,'%') OR address LIKE concat('%',?,'%')";

const char *sql_drop_table(struct subdbinfo *info,
			   const char *name)
{
  sqlite3_stmt *stmt;
  int res;

  if (!stralloc_copys(&line,"DROP TABLE ")) die_nomem();
  if (!stralloc_cats(&line,name)) die_nomem();
  if (!stralloc_0(&line)) die_nomem();

  if ((stmt = _sqlquery(info, &line)) == NULL)
	  return sqlite3_errmsg((sqlite3*)info->conn);

  res = sqlite3_step(stmt);
  sqlite3_finalize(stmt);

  if (res != SQLITE_DONE)
	  return sqlite3_errmsg((sqlite3*)info->conn);
  return 0;
}

struct sub_plugin sub_plugin = {
  SUB_PLUGIN_VERSION,
  sub_sql_checktag,
  _closesub,
  sub_sql_issub,
  _logmsg,
  sub_sql_mktab,
  _opensub,
  sub_sql_putsubs,
  sub_sql_rmtab,
  sub_sql_searchlog,
  _subscribe,
  _tagmsg,
};
