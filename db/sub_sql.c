#include "byte.h"
#include "case.h"
#include "cookie.h"
#include "date822fmt.h"
#include "datetime.h"
#include "die.h"
#include "fmt.h"
#include "log.h"
#include "messages.h"
#include "scan.h"
#include "stralloc.h"
#include "strerr.h"
#include "sub_sql.h"
#include "subdb.h"
#include "subhash.h"

static stralloc addr;
static stralloc name;
static stralloc query;
static stralloc params[4];
static stralloc lcaddr;

static void die_write(void)
{
  strerr_die2sys(111,FATAL,MSG(ERR_WRITE_STDOUT));
}

static void make_name(struct subdbinfo *info,
		      const char *suffix1,
		      const char *suffix2,
		      int terminate)
{
  stralloc_copys(&name,info->base_table);
  if (suffix1) stralloc_cats(&name,suffix1);
  if (suffix2) stralloc_cats(&name,suffix2);
  if (terminate) stralloc_0(&name);
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
  char strnum[FMT_ULONG];

  /* SELECT msgnum FROM table_cookie WHERE msgnum=num and cookie='hash' */
  /* succeeds only is everything correct. */
  if (listno) {			/* only for slaves */
    stralloc_copyb(&params[0],strnum,fmt_ulong(strnum,listno));
    stralloc_copyb(&params[1],strnum,fmt_ulong(strnum,num));
    stralloc_copys(&query,"SELECT listno FROM ");
    stralloc_cats(&query,info->base_table);
    stralloc_cats(&query,"_mlog WHERE ");
    stralloc_cats(&query,sql_checktag_listno_where_defn);

    result = sql_select(info,&query,2,params);
    if (sql_fetch_row(info,result,1,params)) {
      sql_free_result(info,result);
      return "";		/* already done */
    }
    /* no result */
    sql_free_result(info,result);
  }

  stralloc_copyb(&params[0],strnum,fmt_ulong(strnum,num));
  stralloc_copyb(&params[1],hash,COOKIE);

  stralloc_copys(&query,"SELECT msgnum FROM ");
  stralloc_cats(&query,info->base_table);
  stralloc_cats(&query,"_cookie WHERE ");
  stralloc_cats(&query,sql_checktag_msgnum_where_defn);

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
  stralloc_copys(&addr,userhost);
  j = byte_rchr(addr.s,addr.len,'@');
  if (j == addr.len)
    return 0;
  case_lowerb(addr.s + j + 1,addr.len - j - 1);

  stralloc_copys(&query,"SELECT address FROM ");
  stralloc_cat(&query,&name);
  stralloc_cats(&query," WHERE ");
  stralloc_cats(&query,sql_issub_where_defn);

  result = sql_select(info,&query,1,&addr);

  if (!sql_fetch_row(info,result,1,&addr))
    ret = 0;
  else {
    /* we need to return the actual address as other dbs may accept
     * user-*@host, but we still want to make sure to send to e.g the
     * correct moderator address. */
    if (recorded != 0) {
      stralloc_copy(recorded,&addr);
      stralloc_0(recorded);
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
  char strnum[FMT_ULONG];

  stralloc_copys(&query,"INSERT INTO ");
  stralloc_cats(&query,info->base_table);
  stralloc_cats(&query,"_mlog (msgnum,listno,subs,done) VALUES ");
  stralloc_cats(&query,sql_logmsg_values_defn);
  stralloc_copyb(&params[0],strnum,fmt_ulong(strnum,num));
  stralloc_copyb(&params[1],strnum,fmt_ulong(strnum,listno));
  stralloc_copyb(&params[2],strnum,fmt_ulong(strnum,subs));
  s = strnum;
  if (done < 0) {
    done = - done;
    *s++ = '-';
  }
  s[fmt_uint(s,done)] = 0;
  stralloc_copys(&params[3],s);

  sql_exec(info,&query,4,params); /* ignore dups */
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
  char strnum[FMT_ULONG];

  stralloc_copyb(&params[0],strnum,fmt_ulong(strnum,hash_lo));
  stralloc_copyb(&params[1],strnum,fmt_ulong(strnum,hash_hi));
  make_name(info,table?"_":0,table,0);

  /* main query */
  stralloc_copys(&query,"SELECT address FROM ");
  stralloc_cat(&query,&name);
  stralloc_cats(&query," WHERE ");
  stralloc_cats(&query,sql_putsubs_where_defn);

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
  char strnum[FMT_ULONG];

  make_name(info,table?"_":0,table,0);
/* SELECT (*) FROM list_slog WHERE fromline LIKE '%search%' OR address   */
/* LIKE '%search%' ORDER BY tai; */
/* The '*' is formatted to look like the output of the non-mysql version */
/* This requires reading the entire table, since search fields are not   */
/* indexed, but this is a rare query and time is not of the essence.     */

  stralloc_copys(&query,"SELECT ");
  stralloc_cats(&query,sql_searchlog_select_defn);
  stralloc_cats(&query," FROM ");
  stralloc_cat(&query,&name);
  stralloc_cats(&query,"_slog");
  if (*search) {	/* We can afford to wait for LIKE '%xx%' */
    stralloc_copys(&params[0],search);
    stralloc_copys(&params[1],search);
    nparams = 2;
    stralloc_cats(&query," WHERE ");
    stralloc_cats(&query,sql_searchlog_where_defn);
  }
  else
    nparams = 0;
  /* ordering by tai which is an index */
  stralloc_cats(&query," ORDER by tai");

  result = sql_select(info,&query,nparams,params);
  while (sql_fetch_row(info,result,2,params)) {
    stralloc_0(&params[0]);
    (void)scan_ulong(params[0].s,&when);
    datetime_tai(&dt,when);
    stralloc_copyb(&params[0],date,date822fmt(date,&dt)-1);
    stralloc_cats(&params[0],": ");
    stralloc_catb(&params[0],strnum,fmt_ulong(strnum,when));
    stralloc_cats(&params[0]," ");
    stralloc_cat(&params[0],&params[1]);
    if (subwrite(params[0].s,params[0].len) == -1) die_write();
  }
  sql_free_result(info,result);
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
int sub_sql_subscribe(struct subdbinfo *info,
		      const char *table,
		      const char *userhost,
		      int flagadd,
		      const char *comment,
		      const char *event,
		      int forcehash)
{
  void *result;
  char *cpat;
  char szhash[3] = "00";

  unsigned int j;
  unsigned char ch;
  int nparams;

  make_name(info,table?"_":0,table,0);

  /* lowercase and check address */
  stralloc_copys(&addr,userhost);
  if (addr.len > 255)			/* this is 401 in std ezmlm. 255 */
					/* should be plenty! */
    strerr_die2x(100,FATAL,MSG(ERR_ADDR_LONG));
  j = byte_rchr(addr.s,addr.len,'@');
  if (j == addr.len)
    strerr_die2x(100,FATAL,MSG(ERR_ADDR_AT));
  cpat = addr.s + j;
  case_lowerb(cpat + 1,addr.len - j - 1);

  if (forcehash < 0) {
    stralloc_copy(&lcaddr,&addr);
    case_lowerb(lcaddr.s,j);		/* make all-lc version of address */
    ch = subhashsa(&lcaddr);
  } else
    ch = (forcehash % 100);

  szhash[0] = '0' + ch / 10;		/* hash for sublist split */
  szhash[1] = '0' + (ch % 10);

  if (flagadd) {
    /* FIXME: LOCK TABLES name WRITE */
    stralloc_copys(&query,"SELECT address FROM ");
    stralloc_cat(&query,&name);
    stralloc_cats(&query," WHERE ");
    stralloc_cats(&query,sql_subscribe_select_where_defn);
    stralloc_copy(&params[0],&addr);
    result = sql_select(info,&query,1,params);
    if (sql_fetch_row(info,result,1,params)) {
      sql_free_result(info,result);
      /* FIXME: UNLOCK TABLES */
      return 0;			/* already subscribed */
    } else {			/* not there */
      sql_free_result(info,result);

      stralloc_copys(&query,"INSERT INTO ");
      stralloc_cat(&query,&name);
      stralloc_cats(&query," (address,hash) VALUES ");
      stralloc_cats(&query,sql_subscribe_list_values_defn);
      stralloc_copy(&params[0],&addr);
      stralloc_copys(&params[1],szhash);
      sql_exec(info,&query,2,params);
      /* FIXME: UNLOCK TABLES */
    }
  } else {							/* unsub */
    stralloc_copys(&query,"DELETE FROM ");
    stralloc_cat(&query,&name);
    stralloc_cats(&query," WHERE ");
    stralloc_copy(&params[0],&addr);
    if (forcehash >= 0) {
      stralloc_cats(&query,sql_subscribe_delete2_where_defn);
      stralloc_copys(&params[1],szhash);
      nparams = 2;
    }
    else {
      stralloc_cats(&query,sql_subscribe_delete1_where_defn);
      nparams = 1;
    }
    if (sql_exec(info,&query,1,params) == 0)
      return 0;			/* address wasn't there*/
  }

  /* log to subscriber log */
  /* INSERT INTO t_slog (address,edir,etype,fromline) */
  /* VALUES('address',{'+'|'-'},'etype','[comment]') */

  stralloc_copys(&query,"INSERT INTO ");
  stralloc_cat(&query,&name);
  stralloc_cats(&query,"_slog (address,edir,etype,fromline) VALUES ");
  stralloc_cats(&query,sql_subscribe_slog_values_defn);
  stralloc_copy(&params[0],&addr);
  stralloc_copys(&params[1],flagadd?"+":"-"); /* edir */
  stralloc_copyb(&params[2],event+1,!!*(event+1)); /* etype */
  stralloc_copys(&params[3],comment && *comment ? comment : ""); /* from */

  sql_exec(info,&query,4,params); /* log (ignore errors) */
  stralloc_0(&addr);
  logaddr(table,event,addr.s,comment);   /* also log to old log */
  return 1;				 /* desired effect */
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
  char strnum[FMT_ULONG];

  if (chunk >= 53L) chunk = 0L;	/* sanity */

  /* INSERT INTO table_cookie (msgnum,tai,cookie,bodysize,chunk) VALUES (...) */
  /* (we may have tried message before, but failed to complete, so */
  /* ER_DUP_ENTRY is ok) */
  stralloc_copys(&query,"INSERT INTO ");
  stralloc_cats(&query,info->base_table);
  stralloc_cats(&query,"_cookie (msgnum,tai,cookie,bodysize,chunk) VALUES ");
  stralloc_cats(&query,sql_tagmsg_values_defn);
  stralloc_copyb(&params[0],strnum,fmt_ulong(strnum,msgnum));
  stralloc_copyb(&params[1],hashout,COOKIE);
  stralloc_copyb(&params[2],strnum,fmt_ulong(strnum,bodysize));
  stralloc_copyb(&params[3],strnum,fmt_ulong(strnum,chunk));

  sql_exec(info,&query,4,params); /* ignore dups */

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
  stralloc_copys(&query,"CREATE TABLE ");
  stralloc_cats(&query,name.s);
  stralloc_cats(&query," (");
  stralloc_cats(&query,definition);
  stralloc_cats(&query,")");
  stralloc_0(&query);
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
