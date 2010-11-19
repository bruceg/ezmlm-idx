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
#include <mysql/errmsg.h>
#include <mysqld_error.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>

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

static void die_sqlerror(struct subdbinfo *info)
{
  strerr_die2x(111,FATAL,mysql_error((MYSQL*)info->conn));
}

MYSQL_STMT *_prepbind(struct subdbinfo *info,
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
  return stmt;
}

int sql_insert(struct subdbinfo *info,
	       struct stralloc *q,
	       unsigned int nparams,
	       struct stralloc *params)
{
  int rows;
  MYSQL_STMT *stmt;

  stmt = _prepbind(info,q,nparams,params);
  switch (rows = mysql_stmt_execute(stmt)) {
  case 0:
    rows = mysql_stmt_affected_rows(stmt);
    break;
  default:
    if (mysql_stmt_errno(stmt) == ER_DUP_ENTRY)
      rows = 0;
    else
      die_sqlerror(info);
  }
  sql_free_result(info,stmt);
  return rows;
}

void *sql_select(struct subdbinfo *info,
		 struct stralloc *q,
		 unsigned int nparams,
		 struct stralloc *params)
{
  MYSQL_STMT *stmt;

  stmt = _prepbind(info,q,nparams,params);
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

/* Definition of WHERE clauses for checktag */
const char sql_checktag_listno_where_defn[] = "listno=? AND msgnum=? AND done > 3";
const char sql_checktag_msgnum_where_defn[] = "msgnum=? AND cookie=?";

/* Definition of WHERE clause for selecting addresses in issub */
const char sql_issub_where_defn[] = "address LIKE ?";

/* Definition of VALUES clause for insert in logmsg */
const char sql_logmsg_values_defn[] = "(?,?,?,?)";

/* Definition of WHERE clause for selecting addresses in putsubs */
const char sql_putsubs_where_defn[] = "hash BETWEEN ? AND ?";

/* Definition of clauses for searchlog query */
const char sql_searchlog_select_defn[] = "UNIX_TIMESTAMP(tai), CONCAT(edir,etype,' ',address,' ',fromline)";
const char sql_searchlog_where_defn[] = "fromline LIKE concat('%',?,'%') OR address LIKE concat('%',?,'%')";

/* Definition of VALUES clause for insert in tagmsg */
const char sql_tagmsg_values_defn[] = "(?,NOW(),?,?,?)";

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
  sub_sql_checktag,
  _closesub,
  sub_sql_issub,
  sub_sql_logmsg,
  sub_sql_mktab,
  _opensub,
  sub_sql_putsubs,
  sub_sql_rmtab,
  sub_sql_searchlog,
  _subscribe,
  sub_sql_tagmsg,
};
