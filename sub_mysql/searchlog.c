/*$Id: searchlog.c,v 1.15 1999/11/10 04:08:27 lindberg Exp $*/
#include "case.h"
#include "scan.h"
#include "stralloc.h"
#include "str.h"
#include "open.h"
#include "datetime.h"
#include "date822fmt.h"
#include "substdio.h"
#include "readwrite.h"
#include "strerr.h"
#include "error.h"
#include "errtxt.h"
#include "subscribe.h"
#include <mysql.h>

static stralloc line = {0};
static stralloc outline = {0};
static char date[DATE822FMT];
static datetime_sec when;
static struct datetime dt;
static substdio ssin;
static char inbuf[256];

static void die_nomem(fatal)
char *fatal;
{
  strerr_die2x(111,fatal,ERR_NOMEM);
}

static void lineout(subwrite,fatal)
int subwrite();
char *fatal;
{
  (void) scan_ulong(line.s,&when);
  datetime_tai(&dt,when);		/* there is always at least a '\n' */
  if (!stralloc_copyb(&outline,date,date822fmt(date,&dt) - 1))
	die_nomem(fatal);
  if (!stralloc_cats(&outline,": ")) die_nomem(fatal);
  if (!stralloc_catb(&outline,line.s,line.len - 1)) die_nomem(fatal);
  if (subwrite(outline.s,outline.len) == -1)
	strerr_die3x(111,fatal,ERR_WRITE,"output");
  return;
}

void searchlog(dir,search,subwrite,fatal)
/* opens dir/Log, and outputs via subwrite(s,len) any line that matches   */
/* search. A '_' is search is a wildcard. Any other non-alphanum/'.' char */
/* is replaced by a '_'. mysql version. Falls back on "manual" search of  */
/* local Log if no mysql connect info. */

char *dir;		/* work directory */
char *search;		/* search string */
int subwrite();		/* output fxn */
char *fatal;		/* fatal */
{

  register unsigned char x;
  register unsigned char y;
  register unsigned char *cp;
  register unsigned char *cpsearch;
  unsigned register char *cps;
  unsigned register char ch;
  unsigned char *cplast, *cpline;
  unsigned int searchlen;
  int fd,match;
  char *ret;

  MYSQL_RES *result;
  MYSQL_ROW row;
  char *table = (char *) 0;
  char **ptable = &table;
  char *sublist = (char *) 0;
  unsigned long *lengths;

  if (!search) search = "";	/* defensive */
  searchlen = str_len(search);
  case_lowerb(search,searchlen);
  cps = (unsigned char *) search;
  while ((ch = *(cps++))) {	/* search is potentially hostile */
    if (ch >= 'a' && ch <= 'z') continue;
    if (ch >= '0' && ch <= '9') continue;
    if (ch == '.' || ch == '_') continue;
    *(cps - 1) = '_';		/* will match char specified as well */
  }

  if ((ret = opensql(dir,ptable))) {
    if (*ret) strerr_die2x(111,fatal,ret);
						/* fallback to local log */
  if (!stralloc_copys(&line,dir)) die_nomem(fatal);
  if (!stralloc_cats(&line,"/Log")) die_nomem(fatal);
  if (!stralloc_0(&line)) die_nomem(fatal);
  fd = open_read(line.s);
  if (fd == -1)
    if (errno != error_noent)
	strerr_die4sys(111,fatal,ERR_OPEN,line.s,": ");
    else
        strerr_die3x(100,fatal,line.s,ERR_NOEXIST);
  substdio_fdbuf(&ssin,read,fd,inbuf,sizeof(inbuf));

  for (;;) {
    if (getln(&ssin,&line,&match,'\n') == -1)
      strerr_die2sys(111,fatal,ERR_READ_INPUT);
    if (!match) break;
    if (!searchlen) {
      lineout(subwrite,fatal);
    } else {
      cpline = (unsigned char *) line.s - 1;
      cplast = cpline + line.len - searchlen; /* line has \0 at the end */
      while ((cp = ++cpline) <= cplast) {
	cpsearch = (unsigned char *) search;
	for (;;) {
	  x = *cpsearch++;
	  if (!x) break;
	  y = *cp++ - 'A';
	  if (y <= (unsigned char) ('Z' - 'A')) y += 'a'; else y += 'A';
	  if (x != y && x != '_') break;		/* '_' = wildcard */
	}
	if (!x) {
	  lineout(subwrite,fatal);
	  break;
	}
      }
    }
  }
  close(fd);
  } else {

/* SELECT (*) FROM list_slog WHERE fromline LIKE '%search%' OR address   */
/* LIKE '%search%' ORDER BY tai; */
/* The '*' is formatted to look like the output of the non-mysql version */
/* This requires reading the entire table, since search fields are not   */
/* indexed, but this is a rare query and time is not of the essence.     */

    if (!stralloc_cats(&line,"SELECT CONCAT(FROM_UNIXTIME(UNIX_TIMESTAMP(tai)),"
	"'-0000: ',UNIX_TIMESTAMP(tai),' ',edir,etype,' ',address,' ',"
	"fromline) FROM ")) die_nomem(fatal);
    if (!stralloc_cats(&line,table)) die_nomem(fatal);
    if (!stralloc_cats(&line,"_slog ")) die_nomem(fatal);
    if (*search) {	/* We can afford to wait for LIKE '%xx%' */
      if (!stralloc_cats(&line,"WHERE fromline LIKE '%")) die_nomem(fatal);
      if (!stralloc_cats(&line,search)) die_nomem(fatal);
      if (!stralloc_cats(&line,"%' OR address LIKE '%")) die_nomem(fatal);
      if (!stralloc_cats(&line,search)) die_nomem(fatal);
      if (!stralloc_cats(&line,"%'")) die_nomem(fatal);
    }	/* ordering by tai which is an index */
      if (!stralloc_cats(&line," ORDER by tai")) die_nomem(fatal);

    if (mysql_real_query((MYSQL *) psql,line.s,line.len))	/* query */
	strerr_die2x(111,fatal,mysql_error((MYSQL *) psql));
    if (!(result = mysql_use_result((MYSQL *) psql)))
	strerr_die2x(111,fatal,mysql_error((MYSQL *) psql));
    while ((row = mysql_fetch_row(result))) {
    if (!(lengths = mysql_fetch_lengths(result)))
	strerr_die2x(111,fatal,mysql_error((MYSQL *) psql));
      if (subwrite(row[0],lengths[0]) == -1) die_write(fatal);
    }
    if (!mysql_eof(result))
	strerr_die2x(111,fatal,mysql_error((MYSQL *) psql));
    mysql_free_result(result);
  }
}
