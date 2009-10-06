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
#include <libpq-fe.h>
#include <libpq-fe.h> 
#include <stdlib.h>
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

static void dummyNoticeProcessor(void *arg, const char *message)
{
  (void)arg;
  (void)message;
}

/* open connection to the SQL server, if it isn't already open. */
static const char *_opensub(struct subdbinfo *info)
{
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
  if ((PGconn*)info->conn)
    PQfinish((PGconn*)info->conn);
  info->conn = 0;		/* Destroy pointer */
}

/* Checks the hash against the cookie table. If it matches, returns NULL,
 * else returns "". If error, returns error string. */
static const char *_checktag(struct subdbinfo *info,
			     unsigned long num,	/* message number */
			     unsigned long listno, /* bottom of range => slave */
			     const char *action,
			     const char *seed,
			     const char *hash)		/* cookie */
{
  PGresult *result;

    /* SELECT msgnum FROM table_cookie WHERE msgnum=num and cookie='hash' */
    /* succeeds only is everything correct. 'hash' is quoted since it is  */
    /* potentially hostile. */
    if (listno) {			/* only for slaves */
      if (!stralloc_copys(&line,"SELECT listno FROM ")) die_nomem();
      if (!stralloc_cats(&line,info->base_table)) die_nomem();
      if (!stralloc_cats(&line,"_mlog WHERE listno=")) die_nomem();
      if (!stralloc_catb(&line,strnum,fmt_ulong(strnum,listno)))
	die_nomem();
      if (!stralloc_cats(&line," AND msgnum=")) die_nomem();
      if (!stralloc_catb(&line,strnum,fmt_ulong(strnum,num))) die_nomem();
      if (!stralloc_cats(&line," AND done > 3")) die_nomem();

      if (!stralloc_0(&line)) die_nomem();
      result = PQexec( (PGconn*)info->conn, line.s );
      if(result == NULL)
	return (PQerrorMessage((PGconn*)info->conn));
      if( PQresultStatus(result) != PGRES_TUPLES_OK)
	return (char *) (PQresultErrorMessage(result));
      if( PQntuples(result) > 0 ) {
	PQclear(result);
	return("");
      } else
	PQclear(result);
    }

    if (!stralloc_copys(&line,"SELECT msgnum FROM ")) die_nomem();
    if (!stralloc_cats(&line,info->base_table)) die_nomem();
    if (!stralloc_cats(&line,"_cookie WHERE msgnum=")) die_nomem();
    if (!stralloc_catb(&line,strnum,fmt_ulong(strnum,num))) die_nomem();
    if (!stralloc_cats(&line," and cookie='")) die_nomem();
    if (!stralloc_catb(&line,strnum,fmt_str(strnum,hash))) die_nomem();
    if (!stralloc_cats(&line,"'")) die_nomem();

    if (!stralloc_0(&line)) die_nomem();
    result = PQexec((PGconn*)info->conn,line.s);
    if (result == NULL)
      return (PQerrorMessage((PGconn*)info->conn));
    if (PQresultStatus(result) != PGRES_TUPLES_OK)
      return (char *) (PQresultErrorMessage(result));
    if(PQntuples(result) < 0) {
      PQclear( result );
      return("");
    }

    PQclear(result);
    return (char *)0;
    (void)action;
    (void)seed;
}

static int _issub(struct subdbinfo *info,
		  const char *table,
		  const char *userhost,
		  stralloc *recorded)
{
  PGresult *result;

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
    if (!stralloc_cat_table(&line,info,table)) die_nomem();
    if (!stralloc_cats(&line," WHERE address ~* '^")) die_nomem();
    if (!stralloc_cat(&line,&addr)) die_nomem();
    if (!stralloc_cats(&line,"$'")) die_nomem();

    if (!stralloc_0(&line)) die_nomem();
    result = PQexec((PGconn*)info->conn,line.s);
    if (result == NULL)
      strerr_die2x(111,FATAL,PQerrorMessage((PGconn*)info->conn));
    if (PQresultStatus(result) != PGRES_TUPLES_OK )
      strerr_die2x(111,FATAL,PQresultErrorMessage(result));

    /* No data returned in QUERY */
    if (PQntuples(result) < 1) {
      PQclear(result);
      return 0;
    }

    if (recorded) {
      if (!stralloc_copyb(recorded,PQgetvalue(result,0,0),
			  PQgetlength(result,0,0)))
	die_nomem();
      if (!stralloc_0(recorded)) die_nomem();
    }
    PQclear(result);
    return 1;
}

/* Creates an entry for message num and the list listno and code "done".
 * Returns NULL on success, and the error string on error. */
static const char *_logmsg(struct subdbinfo *info,
			   unsigned long num,
			   unsigned long listno,
			   unsigned long subs,
			   int done)
{
  PGresult *result;
  PGresult *result2;

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

  if (!stralloc_0(&logline)) die_nomem();
  result = PQexec((PGconn*)info->conn,logline.s);
  if(result==NULL)
    return (PQerrorMessage((PGconn*)info->conn));
  if(PQresultStatus(result) != PGRES_COMMAND_OK) { /* Check if duplicate */
    if (!stralloc_copys(&logline,"SELECT msgnum FROM ")) die_nomem();
    if (!stralloc_cats(&logline,info->base_table)) die_nomem();
    if (!stralloc_cats(&logline,"_mlog WHERE msgnum = ")) die_nomem();
    if (!stralloc_catb(&logline,strnum,fmt_ulong(strnum,num)))
      die_nomem();
    if (!stralloc_cats(&logline," AND listno = ")) die_nomem();
    if (!stralloc_catb(&logline,strnum,fmt_ulong(strnum,listno)))
      die_nomem();
    if (!stralloc_cats(&logline," AND done = ")) die_nomem();
    if (!stralloc_catb(&logline,strnum,fmt_ulong(strnum,done)))
      die_nomem();
    /* Query */
    if (!stralloc_0(&logline)) die_nomem();
    result2 = PQexec((PGconn*)info->conn,logline.s);
    if (result2 == NULL)
      return (PQerrorMessage((PGconn*)info->conn));
    if (PQresultStatus(result2) != PGRES_TUPLES_OK)
      return (char *) (PQresultErrorMessage(result2));
    /* No duplicate, return ERROR from first query */
    if (PQntuples(result2)<1)
      return (char *) (PQresultErrorMessage(result));
    PQclear(result2);
  }
  PQclear(result);
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
  PGresult *result;
  int row_nr;
  int length;
  char *row;
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
    if (!stralloc_0(&line)) die_nomem();
    result = PQexec((PGconn*)info->conn,line.s);
    if (result == NULL)
      strerr_die2x(111,FATAL,PQerrorMessage((PGconn*)info->conn));
    if (PQresultStatus(result) != PGRES_TUPLES_OK)
      strerr_die2x(111,FATAL,PQresultErrorMessage(result));

    no = 0;
    for (row_nr=0;row_nr<PQntuples(result);row_nr++) {
      /* this is safe even if someone messes with the address field def */
      length = PQgetlength(result,row_nr,0);
      row = PQgetvalue(result,row_nr,0);
      if (subwrite(row,length) == -1) die_write();
      no++;					/* count for list-list fxn */
    }
    PQclear(result);
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
  PGresult *result;
  int row_nr;
  int length;
  char *row;

/* SELECT (*) FROM list_slog WHERE fromline LIKE '%search%' OR address   */
/* LIKE '%search%' ORDER BY tai; */
/* The '*' is formatted to look like the output of the non-mysql version */
/* This requires reading the entire table, since search fields are not   */
/* indexed, but this is a rare query and time is not of the essence.     */
	
    if (!stralloc_copys(&line,"SELECT tai::timestamp||': '"
		       "||extract(epoch from tai)||' '"
		       "||address||' '||edir||etype||' '||fromline FROM "))
      die_nomem();
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
      
    if (!stralloc_0(&line)) die_nomem();  
    result = PQexec((PGconn*)info->conn,line.s);
    if (result == NULL)
      strerr_die2x(111,FATAL,PQerrorMessage((PGconn*)info->conn));
    if (PQresultStatus(result) != PGRES_TUPLES_OK)
      strerr_die2x(111,FATAL,PQresultErrorMessage(result));
    
    for(row_nr=0; row_nr<PQntuples(result); row_nr++) {
      row = PQgetvalue(result,row_nr,0);
      length = PQgetlength(result,row_nr,0);
      if (subwrite(row,length) == -1) die_write();
    }
    PQclear(result);
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
  PGresult *result;
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
    quoted.len = PQescapeString(quoted.s,addr.s,addr.len);
	/* stored unescaped, so it should be ok if quoted.len is >255, as */
	/* long as addr.len is not */

    if (!stralloc_ready(&quoted,2 * addr.len + 1)) die_nomem();
    quoted.len = PQescapeString(quoted.s,addr.s,addr.len);
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
      if (!stralloc_cats(&line," WHERE address ~* '^")) die_nomem();
      if (!stralloc_cat(&line,&quoted)) die_nomem();	/* addr */
      if (!stralloc_cats(&line,"$'")) die_nomem();
      if (!stralloc_0(&line)) die_nomem();
      result = PQexec((PGconn*)info->conn,line.s);
      if (result == NULL)
	strerr_die2x(111,FATAL,PQerrorMessage((PGconn*)info->conn));
      if (PQresultStatus(result) != PGRES_TUPLES_OK)
	strerr_die2x(111,FATAL,PQresultErrorMessage(result));

      if (PQntuples(result)>0) {			/* there */
	PQclear(result);
        return 0;						/* there */
      } else {							/* not there */
	PQclear(result);
	if (!stralloc_copys(&line,"INSERT INTO ")) die_nomem();
	if (!stralloc_cat_table(&line,info,table)) die_nomem();
	if (!stralloc_cats(&line," (address,hash) VALUES ('"))
		die_nomem();
	if (!stralloc_cat(&line,&quoted)) die_nomem();	/* addr */
	if (!stralloc_cats(&line,"',")) die_nomem();
	if (!stralloc_cats(&line,szhash)) die_nomem();	/* hash */
	if (!stralloc_cats(&line,")")) die_nomem();
	if (!stralloc_0(&line)) die_nomem();
	result = PQexec((PGconn*)info->conn,line.s);
	if (result == NULL)
	  strerr_die2x(111,FATAL,PQerrorMessage((PGconn*)info->conn));
	if (PQresultStatus(result) != PGRES_COMMAND_OK)
	  strerr_die2x(111,FATAL,PQresultErrorMessage(result));
      }
    } else {							/* unsub */
      if (!stralloc_copys(&line,"DELETE FROM ")) die_nomem();
      if (!stralloc_cat_table(&line,info,table)) die_nomem();
      if (!stralloc_cats(&line," WHERE address ~* '^")) die_nomem();
      if (!stralloc_cat(&line,&quoted)) die_nomem();	/* addr */
      if (forcehash >= 0) {
	if (!stralloc_cats(&line,"$' AND hash=")) die_nomem();
	if (!stralloc_cats(&line,szhash)) die_nomem();
      } else {
        if (!stralloc_cats(&line,"$' AND hash BETWEEN 0 AND 52"))
		die_nomem();
      }
      
      if (!stralloc_0(&line)) die_nomem();
      result = PQexec((PGconn*)info->conn,line.s);
      if (result == NULL)
	strerr_die2x(111,FATAL,PQerrorMessage((PGconn*)info->conn));
      if (PQresultStatus(result) != PGRES_COMMAND_OK)
	strerr_die2x(111,FATAL,PQresultErrorMessage(result));
      if (atoi(PQcmdTuples(result))<1)
	return 0;				/* address wasn't there*/
      PQclear(result);
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
      quoted.len = PQescapeString(quoted.s,comment,j); /* from */
      if (!stralloc_cat(&logline,&quoted)) die_nomem();
    }
    if (!stralloc_cats(&logline,"')")) die_nomem();

    if (!stralloc_0(&logline)) die_nomem();
    result = PQexec((PGconn*)info->conn,logline.s);		/* log (ignore errors) */
    PQclear(result);

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
  PGresult *result;
  PGresult *result2; /* Need for dupicate check */
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
    
    if (!stralloc_0(&line)) die_nomem();
    result = PQexec((PGconn*)info->conn,line.s);
    if (result == NULL)
      strerr_die2x(111,FATAL,PQerrorMessage((PGconn*)info->conn));
    if (PQresultStatus(result) != PGRES_COMMAND_OK) { /* Possible tuplicate */
      if (!stralloc_copys(&line,"SELECT msgnum FROM ")) die_nomem();
      if (!stralloc_cats(&line,info->base_table)) die_nomem();	  
      if (!stralloc_cats(&line,"_cookie WHERE msgnum = ")) die_nomem();
      if (!stralloc_catb(&line,strnum,fmt_ulong(strnum,msgnum))) 
	die_nomem();
      /* Query */
      if (!stralloc_0(&line)) die_nomem();
      result2 = PQexec((PGconn*)info->conn,line.s);
      if (result2 == NULL)
	strerr_die2x(111,FATAL,PQerrorMessage((PGconn*)info->conn));
      if (PQresultStatus(result2) != PGRES_TUPLES_OK)
	strerr_die2x(111,FATAL,PQresultErrorMessage(result2));
      /* No duplicate, return ERROR from first query */
      if (PQntuples(result2)<1) 
	strerr_die2x(111,FATAL,PQresultErrorMessage(result));
      PQclear(result2);
    }
    PQclear(result);

    if (! (ret = logmsg(msgnum,0L,0L,1))) return;	/* log done=1*/
    if (*ret) strerr_die2x(111,FATAL,ret);
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
  _checktag,
  _closesub,
  _issub,
  _logmsg,
  sub_sql_mktab,
  _opensub,
  _putsubs,
  sub_sql_rmtab,
  _searchlog,
  _subscribe,
  _tagmsg,
};
