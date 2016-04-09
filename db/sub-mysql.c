#include "byte.h"
#include "die.h"
#include "str.h"
#include "strerr.h"
#include "sub_sql.h"
#include "subdb.h"
#include <sys/types.h>
#include <mysql.h>
#include <mysql/errmsg.h>
#include <mysqld_error.h>

static stralloc line = {0};

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

int sql_exec(struct subdbinfo *info,
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
    stralloc_ready(&columns[i],lengths[i]);
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

/* Definition of clauses for subscribe queries */
const char sql_subscribe_select_where_defn[] = "address=?";
const char sql_subscribe_list_values_defn[] = "(?,?)";
const char sql_subscribe_delete1_where_defn[] = "address=? and hash BETWEEN 0 AND 52";
const char sql_subscribe_delete2_where_defn[] = "address=? and hash=?";
const char sql_subscribe_slog_values_defn[] = "(?,?,?,?)";

/* Definition of VALUES clause for insert in tagmsg */
const char sql_tagmsg_values_defn[] = "(?,NOW(),?,?,?)";

int sql_table_exists(struct subdbinfo *info,
		     const char *name)
{
  MYSQL_RES *result;

  stralloc_copys(&line,"SELECT 0 FROM ");
  stralloc_cats(&line,name);
  stralloc_cats(&line," LIMIT 1");
  stralloc_0(&line);
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
  stralloc_copys(&line,"DROP TABLE ");
  stralloc_cats(&line,name);
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
  sub_sql_subscribe,
  sub_sql_tagmsg,
};
