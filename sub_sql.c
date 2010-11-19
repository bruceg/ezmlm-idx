#include "byte.h"
#include "case.h"
#include "cookie.h"
#include "date822fmt.h"
#include "datetime.h"
#include "die.h"
#include "fmt.h"
#include "messages.h"
#include "scan.h"
#include "stralloc.h"
#include "strerr.h"
#include "sub_sql.h"
#include "subdb.h"

static stralloc addr;
static stralloc name;
static stralloc query;
static stralloc params[4];
static char strnum[FMT_ULONG];

static void die_write(void)
{
  strerr_die2sys(111,FATAL,MSG(ERR_WRITE_STDOUT));
}

static void make_name(struct subdbinfo *info,
		      const char *suffix1,
		      const char *suffix2,
		      int terminate)
{
  if (!stralloc_copys(&name,info->base_table)) die_nomem();
  if (suffix1 && !stralloc_cats(&name,suffix1)) die_nomem();
  if (suffix2 && !stralloc_cats(&name,suffix2)) die_nomem();
  if (terminate && !stralloc_0(&name)) die_nomem();
}

/* Checks the hash against the cookie table. If it matches, returns NULL,
 * else returns "". If error, returns error string. */
const char *sub_sql_checktag (struct subdbinfo *info,
			      unsigned long num,	/* message number */
			      unsigned long listno,	/* bottom of range => slave */
			      const char *action,
			      const char *seed,
			      const char *hash)		/* cookie */
{
  void *result;

  /* SELECT msgnum FROM table_cookie WHERE msgnum=num and cookie='hash' */
  /* succeeds only is everything correct. */
  if (listno) {			/* only for slaves */
    if (!stralloc_copyb(&params[0],strnum,fmt_ulong(strnum,listno))) die_nomem();
    if (!stralloc_copyb(&params[1],strnum,fmt_ulong(strnum,num))) die_nomem();
    if (!stralloc_copys(&query,"SELECT listno FROM ")) die_nomem();
    if (!stralloc_cats(&query,info->base_table)) die_nomem();
    if (!stralloc_cats(&query,"_mlog WHERE ")) die_nomem();
    if (!stralloc_cats(&query,sql_checktag_listno_where_defn)) die_nomem();

    result = sql_select(info,&query,2,params);
    if (sql_fetch_row(info,result,1,params)) {
      sql_free_result(info,result);
      return "";		/* already done */
    }
    /* no result */
    sql_free_result(info,result);
  }

  if (!stralloc_copyb(&params[0],strnum,fmt_ulong(strnum,num))) die_nomem();
  if (!stralloc_copyb(&params[1],hash,COOKIE)) die_nomem();

  if (!stralloc_copys(&query,"SELECT msgnum FROM ")) die_nomem();
  if (!stralloc_cats(&query,info->base_table)) die_nomem();
  if (!stralloc_cats(&query,"_cookie WHERE ")) die_nomem();
  if (!stralloc_cats(&query,sql_checktag_msgnum_where_defn)) die_nomem();

  result = sql_select(info,&query,2,params);
  if (!sql_fetch_row(info,result,1,params)) {
    sql_free_result(info,result);
    return "";			/* not parent => perm error */
  }
  sql_free_result(info,result);	/* success! cookie matches */
  return (char *)0;
  (void)action;
  (void)seed;
}

int sub_sql_issub(struct subdbinfo *info,
		  const char *table,
		  const char *userhost,
		  stralloc *recorded)
{
  unsigned int j;
  void *result;
  int ret;

  /* SELECT address FROM list WHERE address = 'userhost' AND hash */
  /* BETWEEN 0 AND 52. Without the hash restriction, we'd make it */
  /* even easier to defeat. Just faking sender to the list name would*/
  /* work. Since sender checks for posts are bogus anyway, I don't */
  /* know if it's worth the cost of the "WHERE ...". */

  make_name(info,table?"_":0,table,0);

  /* Lower-case the domain portion */
  if (!stralloc_copys(&addr,userhost)) die_nomem();
  j = byte_rchr(addr.s,addr.len,'@');
  if (j == addr.len)
    return 0;
  case_lowerb(addr.s + j + 1,addr.len - j - 1);

  if (!stralloc_copys(&query,"SELECT address FROM ")) die_nomem();
  if (!stralloc_cat(&query,&name)) die_nomem();
  if (!stralloc_cats(&query," WHERE ")) die_nomem();
  if (!stralloc_cats(&query,sql_issub_where_defn)) die_nomem();

  result = sql_select(info,&query,1,&addr);

  if (!sql_fetch_row(info,result,1,&addr))
    ret = 0;
  else {
    /* we need to return the actual address as other dbs may accept
     * user-*@host, but we still want to make sure to send to e.g the
     * correct moderator address. */
    if (recorded != 0) {
      if (!stralloc_copy(recorded,&addr)) die_nomem();
      if (!stralloc_0(recorded)) die_nomem();
    }
    ret = 1;
  }
  sql_free_result(info,result);
  return ret;
}

/* Creates an entry for message num and the list listno and code "done".
 * Returns NULL on success, and the error string on error. */
const char *sub_sql_logmsg(struct subdbinfo *info,
			   unsigned long num,
			   unsigned long listno,
			   unsigned long subs,
			   int done)
{
  char *s;

  if (!stralloc_copys(&query,"INSERT INTO ")) die_nomem();
  if (!stralloc_cats(&query,info->base_table)) die_nomem();
  if (!stralloc_cats(&query,"_mlog (msgnum,listno,subs,done) VALUES "))
	die_nomem();
  if (!stralloc_cats(&query,sql_logmsg_values_defn)) die_nomem();
  if (!stralloc_copyb(&params[0],strnum,fmt_ulong(strnum,num))) die_nomem();
  if (!stralloc_copyb(&params[1],strnum,fmt_ulong(strnum,listno)))
	die_nomem();
  if (!stralloc_copyb(&params[2],strnum,fmt_ulong(strnum,subs))) die_nomem();
  s = strnum;
  if (done < 0) {
    done = - done;
    *s++ = '-';
  }
  s[fmt_uint(s,done)] = 0;
  if (!stralloc_copys(&params[3],s)) die_nomem();

  sql_insert(info,&query,4,params); /* ignore dups */
  return 0;
}

/* Outputs all addresses in the table through subwrite. subwrite must be
 * a function returning >=0 on success, -1 on error, and taking
 * arguments (char* string, unsigned int length). It will be called once
 * per address and should take care of newline or whatever needed for
 * the output form. */
unsigned long sub_sql_putsubs(struct subdbinfo *info,
			      const char *table,
			      unsigned long hash_lo,
			      unsigned long hash_hi,
			      int subwrite()) /* write function. */
{
  void *result;
  unsigned long no = 0L;

  if (!stralloc_copyb(&params[0],strnum,fmt_ulong(strnum,hash_lo))) die_nomem();
  if (!stralloc_copyb(&params[1],strnum,fmt_ulong(strnum,hash_hi))) die_nomem();
  make_name(info,table?"_":0,table,0);

  /* main query */
  if (!stralloc_copys(&query,"SELECT address FROM "))
		die_nomem();
  if (!stralloc_cat(&query,&name)) die_nomem();
  if (!stralloc_cats(&query," WHERE ")) die_nomem();
  if (!stralloc_cats(&query,sql_putsubs_where_defn)) die_nomem();

  result = sql_select(info,&query,2,params);

  no = 0;
  while (sql_fetch_row(info,result,1,&addr)) {
    if (subwrite(addr.s,addr.len) == -1) die_write();
    no++;					/* count for list-list fxn */
  }
  sql_free_result(info,result);
  return no;
}

/* Searches the subscriber log and outputs via subwrite(s,len) any entry
 * that matches search. A '_' is search is a wildcard. Any other
 * non-alphanum/'.' char is replaced by a '_'. */
void sub_sql_searchlog(struct subdbinfo *info,
		       const char *table,
		       char *search,		/* search string */
		       int subwrite())		/* output fxn */
{
  void *result;
  datetime_sec when;
  struct datetime dt;
  char date[DATE822FMT];
  int nparams;

  make_name(info,table?"_":0,table,0);
/* SELECT (*) FROM list_slog WHERE fromline LIKE '%search%' OR address   */
/* LIKE '%search%' ORDER BY tai; */
/* The '*' is formatted to look like the output of the non-mysql version */
/* This requires reading the entire table, since search fields are not   */
/* indexed, but this is a rare query and time is not of the essence.     */

  if (!stralloc_copys(&query,"SELECT ")) die_nomem();
  if (!stralloc_cats(&query,sql_searchlog_select_defn)) die_nomem();
  if (!stralloc_cats(&query," FROM ")) die_nomem();
  if (!stralloc_cat(&query,&name)) die_nomem();
  if (!stralloc_cats(&query,"_slog")) die_nomem();
  if (*search) {	/* We can afford to wait for LIKE '%xx%' */
    if (!stralloc_copys(&params[0],search)) die_nomem();
    if (!stralloc_copys(&params[1],search)) die_nomem();
    nparams = 2;
    if (!stralloc_cats(&query," WHERE ")) die_nomem();
    if (!stralloc_cats(&query,sql_searchlog_where_defn)) die_nomem();
  }
  else
    nparams = 0;
  /* ordering by tai which is an index */
  if (!stralloc_cats(&query," ORDER by tai")) die_nomem();

  result = sql_select(info,&query,nparams,params);
  while (sql_fetch_row(info,result,2,params)) {
    if (!stralloc_0(&params[0])) die_nomem();
    (void)scan_ulong(params[0].s,&when);
    datetime_tai(&dt,when);
    if (!stralloc_copyb(&params[0],date,date822fmt(date,&dt)-1)) die_nomem();
    if (!stralloc_cats(&params[0],": ")) die_nomem();
    if (!stralloc_catb(&params[0],strnum,fmt_ulong(strnum,when))) die_nomem();
    if (!stralloc_cats(&params[0]," ")) die_nomem();
    if (!stralloc_cat(&params[0],&params[1])) die_nomem();
    if (subwrite(params[0].s,params[0].len) == -1) die_write();
  }
  sql_free_result(info,result);
}

/* This routine inserts the cookie into table_cookie. We log arrival of
 * the message (done=0). */
void sub_sql_tagmsg(struct subdbinfo *info,
		    unsigned long msgnum,	/* number of this message */
		    const char *hashout,	/* previously calculated hash */
		    unsigned long bodysize,
		    unsigned long chunk)
{
  const char *ret;

  if (chunk >= 53L) chunk = 0L;	/* sanity */

  /* INSERT INTO table_cookie (msgnum,tai,cookie,bodysize,chunk) VALUES (...) */
  /* (we may have tried message before, but failed to complete, so */
  /* ER_DUP_ENTRY is ok) */
  if (!stralloc_copys(&query,"INSERT INTO ")) die_nomem();
  if (!stralloc_cats(&query,info->base_table)) die_nomem();
  if (!stralloc_cats(&query,"_cookie (msgnum,tai,cookie,bodysize,chunk) VALUES "))
    die_nomem();
  if (!stralloc_cats(&query,sql_tagmsg_values_defn)) die_nomem();
  if (!stralloc_copyb(&params[0],strnum,fmt_ulong(strnum,msgnum))) die_nomem();
  if (!stralloc_copyb(&params[1],hashout,COOKIE)) die_nomem();
  if (!stralloc_copyb(&params[2],strnum,fmt_ulong(strnum,bodysize))) die_nomem();
  if (!stralloc_copyb(&params[3],strnum,fmt_ulong(strnum,chunk))) die_nomem();

  sql_insert(info,&query,4,params); /* ignore dups */

  if (! (ret = logmsg(msgnum,0L,0L,1)))
    return;			/* log done=1*/
  if (*ret) strerr_die2x(111,FATAL,ret);
}

static const char *create_table(struct subdbinfo *info,
				const char *suffix1,
				const char *suffix2,
				const char *definition)
{
  make_name(info,suffix1,suffix2,1);
  if (sql_table_exists(info,name.s) > 0)
    return 0;
  if (!stralloc_copys(&query,"CREATE TABLE ")) die_nomem();
  if (!stralloc_cats(&query,name.s)) die_nomem();
  if (!stralloc_cats(&query," (")) die_nomem();
  if (!stralloc_cats(&query,definition)) die_nomem();
  if (!stralloc_cats(&query,")")) die_nomem();
  if (!stralloc_0(&query)) die_nomem();
  return sql_create_table(info,query.s);
}

static const char *create_table_set(struct subdbinfo *info,
				    const char *suffix,
				    int do_mlog)
{
  const char *r;

  /* Address table */
  if ((r = create_table(info,suffix,"",sql_sub_table_defn)) != 0)
    return r;
  /* Subscription log table. */
  if ((r = create_table(info,suffix,"_slog",sql_slog_table_defn)) != 0)
    return r;

  if (do_mlog) {
    /* main list inserts a cookie here. Sublists check it */
    if ((r = create_table(info,suffix,"_cookie",sql_cookie_table_defn)) != 0)
      return r;

    /* main and sublist log here when the message is done done=0 for
     * arrived, done=4 for sent, 5 for receit. */
    if ((r = create_table(info,suffix,"_mlog",sql_mlog_table_defn)) != 0)
      return r;
  }

  return 0;
}

const char *sub_sql_mktab(struct subdbinfo *info)
{
  const char *r;

  if ((r = create_table_set(info,"",1)) != 0)
    return r;
  if ((r = create_table_set(info,"_allow",0)) != 0)
    return r;
  if ((r = create_table_set(info,"_deny",0)) != 0)
    return r;
  if ((r = create_table_set(info,"_digest",1)) != 0)
    return r;
  if ((r = create_table_set(info,"_mod",0)) != 0)
    return r;
  return 0;
}

static const char *remove_table(struct subdbinfo *info,
				const char *suffix1,
				const char *suffix2)
{
  make_name(info,suffix1,suffix2,1);
  if (sql_table_exists(info,name.s) == 0)
    return 0;
  return sql_drop_table(info,name.s);
}

static const char *remove_table_set(struct subdbinfo *info,
				    const char *suffix)
{
  const char *r;
  if ((r = remove_table(info,suffix,"_mlog")) != 0
      || (r = remove_table(info,suffix,"_cookie")) != 0
      || (r = remove_table(info,suffix,"_slog")) != 0
      || (r = remove_table(info,suffix,"")) != 0)
    return r;
  return 0;
}

const char *sub_sql_rmtab(struct subdbinfo *info)
{
  const char *r;
  if ((r = remove_table_set(info,"")) != 0
      || (r = remove_table_set(info,"_allow")) != 0
      || (r = remove_table_set(info,"_deny")) != 0
      || (r = remove_table_set(info,"_digest")) != 0
      || (r = remove_table_set(info,"_mod")) != 0)
    return r;
  return 0;
}
