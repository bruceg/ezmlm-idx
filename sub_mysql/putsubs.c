/*$Id$*/

#include <unistd.h>
#include "getln.h"
#include "error.h"
#include "strerr.h"
#include "readwrite.h"
#include "str.h"
#include "fmt.h"
#include "stralloc.h"
#include "open.h"
#include "substdio.h"
#include "case.h"
#include "errtxt.h"
#include "subscribe.h"
#include "qmail.h"
#include <mysql.h>

static substdio ssin;
static char inbuf[512];
char strnum[FMT_ULONG];
static stralloc line = {0};
static stralloc fn = {0};

static void die_nomem(fatal)
char *fatal;
{
  strerr_die2x(111,fatal,ERR_NOMEM);
}

static void die_write(fatal)
char *fatal;
{
  strerr_die3x(111,fatal,ERR_WRITE,"stdout");
}

unsigned long putsubs(dbname,hash_lo,hash_hi,
	subwrite,flagsql,fatal)
/* Outputs all userhostesses in 'dbname' to stdout. If userhost is not null */
/* that userhost is excluded. 'dbname' is the base directory name. For the  */
/* mysql version, dbname is the directory where the file "sql" with mysql   */
/* access info is found. If this file is not present or if flagmysql is not */
/* set, the routine falls back to the old database style. subwrite must be a*/
/* function returning >=0 on success, -1 on error, and taking arguments     */
/* (char* string, unsigned int length). It will be called once per address  */
/* and should take care of newline or whatever needed for the output form.  */

char *dbname;		/* database base dir */
unsigned long hash_lo;
unsigned long hash_hi;
int subwrite();		/* write function. */
int flagsql;
char *fatal;		/* fatal error string */

{
  MYSQL_RES *result;
  MYSQL_ROW row;
  char *table = (char *) 0;
  unsigned long *lengths;

  unsigned int i;
  int fd;
  unsigned long no = 0L;
  int match;
  unsigned int hashpos;
  const char *ret = (char *) 0;

  if (!flagsql || (ret = opensql(dbname,&table))) {
    if (flagsql && *ret) strerr_die2x(111,fatal,ret);
						/* fallback to local db */
    if (!stralloc_copys(&fn,dbname)) die_nomem(fatal);
    if (!stralloc_catb(&fn,"/subscribers/?",15)) die_nomem(fatal);
				/* NOTE: Also copies terminal '\0' */
    hashpos = fn.len - 2;
    if (hash_lo > 52) hash_lo = 52;
    if (hash_hi > 52) hash_hi = 52;
    if (hash_hi < hash_lo) hash_hi = hash_lo;

    for (i = hash_lo;i <= hash_hi;++i) {
      fn.s[hashpos] = 64 + i;	/* hash range 0-52 */
      fd = open_read(fn.s);
      if (fd == -1) {
        if (errno != error_noent)
	  strerr_die4sys(111,fatal,ERR_READ,fn.s,": ");
      } else {
        substdio_fdbuf(&ssin,read,fd,inbuf,sizeof(inbuf));
        for (;;) {
          if (getln(&ssin,&line,&match,'\0') == -1)
            strerr_die4sys(111,fatal,ERR_READ,fn.s,": ");
          if (!match)
            break;
          if (subwrite(line.s + 1,line.len - 2) == -1) die_write(fatal);
          no++;
        }
        close(fd);
      }
    }
    return no;

  } else {					/* SQL Version */

						/* main query */
    if (!stralloc_copys(&line,"SELECT address FROM "))
		die_nomem(fatal);
    if (!stralloc_cats(&line,table)) die_nomem(fatal);
    if (!stralloc_cats(&line," WHERE hash BETWEEN ")) die_nomem(fatal);
    if (!stralloc_catb(&line,strnum,fmt_ulong(strnum,hash_lo)))
		die_nomem(fatal);
    if (!stralloc_cats(&line," AND ")) die_nomem(fatal);
    if (!stralloc_catb(&line,strnum,fmt_ulong(strnum,hash_hi)))
		die_nomem(fatal);
    if (mysql_real_query((MYSQL *) psql,line.s,line.len))	/* query */
	strerr_die2x(111,fatal,mysql_error((MYSQL *) psql));
    if (!(result = mysql_use_result((MYSQL *) psql)))
	strerr_die2x(111,fatal,mysql_error((MYSQL *) psql));
    no = 0;
    while ((row = mysql_fetch_row(result))) {
	/* this is safe even if someone messes with the address field def */
    if (!(lengths = mysql_fetch_lengths(result)))
	strerr_die2x(111,fatal,mysql_error((MYSQL *) psql));
      if (subwrite(row[0],lengths[0]) == -1) die_write(fatal);
      no++;					/* count for list-list fxn */
    }
    if (!mysql_eof(result))
	strerr_die2x(111,fatal,mysql_error((MYSQL *) psql));
    mysql_free_result(result);
    return no;
  }
}
