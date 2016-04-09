#include "die.h"
#include "fmt.h"
#include "scan.h"
#include "strerr.h"
#include "sub_sql.h"
#include <sys/types.h>
#include <libpq-fe.h>
#include <stdlib.h>
#include <string.h>

static stralloc line = {0};

struct _result
{
  PGresult *res;
  int row;
  int nrows;
};

static void dummyNoticeProcessor(void *arg, const char *message)
{
  (void)arg;
  (void)message;
}

/* open connection to the SQL server, if it isn't already open. */
static const char *_opensub(struct subdbinfo *info)
{
  char strnum[FMT_ULONG];
  if (info->conn == 0) {
    /* Make connection to database */
    strnum[fmt_ulong(strnum,info->port)] = 0;
    info->conn = PQsetdbLogin(info->host,info->port?strnum:"",NULL,NULL,
			      info->db,info->user,info->pw);
    /* Check  to see that the backend connection was successfully made */
    if (PQstatus((PGconn*)info->conn) == CONNECTION_BAD)
      return PQerrorMessage((PGconn*)info->conn);
    /* Suppress output of notices. */
    PQsetNoticeProcessor((PGconn*)info->conn, dummyNoticeProcessor, NULL);
  }
  return (char *) 0;
}

/* close connection to SQL server, if open */
static void _closesub(struct subdbinfo *info)
{
  if ((PGconn*)info->conn)
    PQfinish((PGconn*)info->conn);
  info->conn = 0;		/* Destroy pointer */
}

static void die_sqlerror(struct subdbinfo *info)
{
  strerr_die2x(111,FATAL,PQerrorMessage((PGconn*)info->conn));
}

static void die_sqlresulterror(PGresult *result)
{
  strerr_die2x(111,FATAL,PQresultErrorMessage(result));
}

PGresult *_execute(struct subdbinfo *info,
		   struct stralloc *q,
		   unsigned int nparams,
		   struct stralloc *params)
{
  PGresult *result;
  const char *values[nparams];
  int lengths[nparams];
  unsigned int i;

  if (!stralloc_0(q)) die_nomem();
  for (i = 0; i < nparams; ++i) {
    if (!stralloc_0(&params[i])) die_nomem();
    values[i] = params[i].s;
    lengths[i] = params[i].len - 1;
  }

  result = PQexecParams((PGconn*)info->conn,q->s,nparams,0,values,lengths,0,0);
  if (result == 0)
    die_sqlerror(info);
  return result;
}

int sql_exec(struct subdbinfo *info,
	     struct stralloc *q,
	     unsigned int nparams,
	     struct stralloc *params)
{
  PGresult *result;
  unsigned long rows;
  const char *err;

  result = _execute(info,q,nparams,params);
  switch (PQresultStatus(result)) {
  case PGRES_COMMAND_OK:
    err = PQcmdTuples(result);
    (void)scan_ulong(err,&rows);
    break;
  default:
    /* This is ugly, but I can't find another good way of doing this */
    err = PQresultErrorMessage(result);
    if (strstr(err, "duplicate") != 0)
      rows = 0;
    else
      die_sqlresulterror(result);
  }
  PQclear(result);
  return rows;
}

void *sql_select(struct subdbinfo *info,
		 struct stralloc *q,
		 unsigned int nparams,
		 struct stralloc *params)
{
  PGresult *result;
  struct _result *ret;

  ret = alloc(sizeof *ret);

  result = _execute(info,q,nparams,params);
  if (PQresultStatus(result) != PGRES_TUPLES_OK)
    die_sqlresulterror(result);

  ret->res = result;
  ret->row = 0;
  ret->nrows = PQntuples(result);
  return ret;
}

int sql_fetch_row(struct subdbinfo *info,
		  void *result,
		  unsigned int ncolumns,
		  struct stralloc *columns)
{
  struct _result *r = result;
  unsigned int i;

  if (r->row >= r->nrows)
    return 0;

  for (i = 0; i < ncolumns; ++i) {
    if (!stralloc_copys(&columns[i],PQgetvalue(r->res,r->row,i)))
      die_nomem();
  }
  ++r->row;
  return 1;
  (void)info;
}

extern void sql_free_result(struct subdbinfo *info,
			    void *result)
{
  struct _result *r = result;
  PQclear(r->res);
  free(r);
  (void)info;
}

int sql_table_exists(struct subdbinfo *info,
		     const char *name)
{
  PGresult *result;

  /* This is a very crude cross-version portable kludge to test if a
   * table exists by testing if a select from the table succeeds.  */
  if (!stralloc_copys(&line,"SELECT 0 FROM ")) return -1;
  if (!stralloc_cats(&line,name)) return -1;
  if (!stralloc_cats(&line," LIMIT 1")) return -1;
  if (!stralloc_0(&line)) return -1;
  result = PQexec((PGconn*)info->conn,line.s);
  if (result == NULL)
    return -1;
  if (PQresultStatus(result) == PGRES_TUPLES_OK) {
    PQclear(result);
    return 1;
  }
  PQclear(result);
  return 0;
}

const char *sql_create_table(struct subdbinfo *info,
			     const char *definition)
{
  PGresult *result;

  result = PQexec((PGconn*)info->conn,definition);
  if (result == NULL)
    return PQerrorMessage((PGconn*)info->conn);
  if (PQresultStatus(result) != PGRES_COMMAND_OK)
    return PQresultErrorMessage(result);
  PQclear(result);
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
  "  tai	TIMESTAMP DEFAULT now(),"
  "  address	VARCHAR(255) NOT NULL,"
  "  fromline	VARCHAR(255) NOT NULL,"
  "  edir	CHAR NOT NULL,"
  "  etype	CHAR NOT NULL";

/* main list inserts a cookie here. Sublists check it */
const char sql_cookie_table_defn[] =
  "  msgnum	INT4 NOT NULL PRIMARY KEY,"
  "  tai	TIMESTAMP NOT NULL DEFAULT now(),"
  "  cookie	CHAR(20) NOT NULL,"
  "  chunk	INT4 NOT NULL DEFAULT 0,"
  "  bodysize	INT4 NOT NULL DEFAULT 0";

/* main and sublist log here when the message is done done=0 for
 * arrived, done=4 for sent, 5 for receit.  tai reflects last change */
const char sql_mlog_table_defn[] =
  "msgnum	INT4 NOT NULL,"
  "listno	INT4 NOT NULL,"
  "tai		TIMESTAMP DEFAULT now(),"
  "subs		INT4 NOT NULL DEFAULT 0,"
  "done		INT4 NOT NULL DEFAULT 0,"
  "PRIMARY KEY (listno,msgnum,done)";

/* Definition of WHERE clauses for checktag */
const char sql_checktag_listno_where_defn[] = "listno=$1 AND msgnum=$2 AND done > 3";
const char sql_checktag_msgnum_where_defn[] = "msgnum=$1 AND cookie=$2";

/* Definition of WHERE clause for selecting addresses in issub */
const char sql_issub_where_defn[] = "address ~* ('^' || $1 || '$')";

/* Definition of VALUES clause for insert in logmsg */
const char sql_logmsg_values_defn[] = "($1,$2,$3,$4)";

/* Definition of WHERE clause for selecting addresses in putsubs */
const char sql_putsubs_where_defn[] = "hash BETWEEN $1 AND $2";

/* Definition of clauses for searchlog query */
const char sql_searchlog_select_defn[] = "extract(epoch from tai),address||' '||edir||etype||' '||fromline";
const char sql_searchlog_where_defn[] = "fromline LIKE concat('%',$1,'%') OR address LIKE concat('%',$1,'%')";

/* Definition of clauses for subscribe queries */
const char sql_subscribe_select_where_defn[] = "address=$1";
const char sql_subscribe_list_values_defn[] = "($1,$2)";
const char sql_subscribe_delete1_where_defn[] = "address ~* ('^' || $1 || '$') and hash BETWEEN 0 AND 52";
const char sql_subscribe_delete2_where_defn[] = "address ~* ('^' || $1 || '$') and hash=$2";
const char sql_subscribe_slog_values_defn[] = "($1,$2,$3,$4)";

/* Definition of VALUES clause for insert in tagmsg */
const char sql_tagmsg_values_defn[] = "($1,NOW(),$2,$3,$4)";

const char *sql_drop_table(struct subdbinfo *info,
			   const char *name)
{
  PGresult *result;

  if (!stralloc_copys(&line,"DROP TABLE ")) die_nomem();
  if (!stralloc_cats(&line,name)) die_nomem();
  if (!stralloc_0(&line)) die_nomem();
  result = PQexec((PGconn*)info->conn,line.s);
  if (result == NULL)
    return PQerrorMessage((PGconn*)info->conn);
  if (PQresultStatus(result) != PGRES_COMMAND_OK)
    return PQresultErrorMessage(result);
  PQclear(result);
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
