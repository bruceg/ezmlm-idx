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

static stralloc line = {0};

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

static void die_sqlerror(struct subdbinfo *info)
{
  strerr_die2x(111,FATAL,sqlite3_errmsg((sqlite3*)info->conn));
}

int sql_exec(struct subdbinfo *info,
	     struct stralloc *q,
	     unsigned int nparams,
	     struct stralloc *params)
{
  sqlite3_stmt *stmt;
  int rows = 0;

  stmt = sql_select(info,q,nparams,params);
  switch (sqlite3_step(stmt)) {
  case SQLITE_DONE:
    rows = sqlite3_changes((sqlite3*)info->conn);
    break;
  case SQLITE_CONSTRAINT:	/* duplicated key on insert */
    break;
  default:
    die_sqlerror(info);
  }
  sqlite3_finalize(stmt);
  return rows;
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

/* Definition of VALUES clause for insert in logmsg */
const char sql_logmsg_values_defn[] = "(?,?,?,?)";

/* Definition of WHERE clause for selecting addresses in putsubs */
const char sql_putsubs_where_defn[] = "hash BETWEEN ? AND ?";

/* Definition of clauses for searchlog query */
const char sql_searchlog_select_defn[] = "tai, edir||etype||' '||address||' '||fromline";
const char sql_searchlog_where_defn[] = "fromline LIKE concat('%',?,'%') OR address LIKE concat('%',?,'%')";

/* Definition of clauses for subscribe queries */
const char sql_subscribe_select_where_defn[] = "address=?";
const char sql_subscribe_list_values_defn[] = "(?,?)";
const char sql_subscribe_delete1_where_defn[] = "address LIKE ? and hash BETWEEN 0 AND 52";
const char sql_subscribe_delete2_where_defn[] = "address LIKE ? and hash=?";
const char sql_subscribe_slog_values_defn[] = "(?,?,?,?)";

/* Definition of VALUES clause for insert in tagmsg */
const char sql_tagmsg_values_defn[] = "(?,'now',?,?,?)";

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
  sub_sql_logmsg,
  sub_sql_mktab,
  _opensub,
  sub_sql_putsubs,
  sub_sql_rmtab,
  sub_sql_searchlog,
  sub_sql_subscribe,
  sub_sql_tagmsg,
};
