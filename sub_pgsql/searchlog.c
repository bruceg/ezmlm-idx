/*$Id$*/

#include "getln.h"
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
#include "idx.h"
#include <unistd.h>
#include <libpq-fe.h>

extern void die_write(void);

static stralloc line = {0};
static stralloc outline = {0};
static char date[DATE822FMT];
static datetime_sec when;
static struct datetime dt;
static substdio ssin;
static char inbuf[256];

static void lineout(int subwrite())
{
  (void) scan_ulong(line.s,&when);
  datetime_tai(&dt,when);		/* there is always at least a '\n' */
  if (!stralloc_copyb(&outline,date,date822fmt(date,&dt) - 1))
	die_nomem();
  if (!stralloc_cats(&outline,": ")) die_nomem();
  if (!stralloc_catb(&outline,line.s,line.len - 1)) die_nomem();
  if (subwrite(outline.s,outline.len) == -1)
	strerr_die3x(111,FATAL,ERR_WRITE,"output");
  return;
}

void searchlog(const char *dir,		/* work directory */
	       char *search,		/* search string */
	       int subwrite())		/* output fxn */
/* opens dir/Log, and outputs via subwrite(s,len) any line that matches   */
/* search. A '_' is search is a wildcard. Any other non-alphanum/'.' char */
/* is replaced by a '_'. mysql version. Falls back on "manual" search of  */
/* local Log if no mysql connect info. */
{

  unsigned char x;
  unsigned char y;
  unsigned char *cp;
  unsigned char *cpsearch;
  unsigned char *cps;
  unsigned char ch;
  unsigned char *cplast, *cpline;
  unsigned int searchlen;
  int fd,match;
  const char *ret;

  PGresult *result;
  int row_nr;
  int length;
  char *row;

  const char *table = (char *) 0;

  if (!search) search = (char*)"";	/* defensive */
  searchlen = str_len(search);
  case_lowerb(search,searchlen);
  cps = (unsigned char *) search;
  while ((ch = *(cps++))) {	/* search is potentially hostile */
    if (ch >= 'a' && ch <= 'z') continue;
    if (ch >= '0' && ch <= '9') continue;
    if (ch == '.' || ch == '_') continue;
    *(cps - 1) = '_';		/* will match char specified as well */
  }

  if ((ret = opensql(dir,&table))) {
    if (*ret) strerr_die2x(111,FATAL,ret);
						/* fallback to local log */
  if (!stralloc_copys(&line,dir)) die_nomem();
  if (!stralloc_cats(&line,"/Log")) die_nomem();
  if (!stralloc_0(&line)) die_nomem();
  fd = open_read(line.s);
  if (fd == -1)
    if (errno != error_noent)
	strerr_die4sys(111,FATAL,ERR_OPEN,line.s,": ");
    else
        strerr_die3x(100,FATAL,line.s,ERR_NOEXIST);
  substdio_fdbuf(&ssin,read,fd,inbuf,sizeof(inbuf));

  for (;;) {
    if (getln(&ssin,&line,&match,'\n') == -1)
      strerr_die2sys(111,FATAL,ERR_READ_INPUT);
    if (!match) break;
    if (!searchlen) {
      lineout(subwrite);
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
	  lineout(subwrite);
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
	
    if (!stralloc_cats(&line,"SELECT tai::datetime||': '||tai::int8||' '"
    "||address||' '||edir||etype||' '||fromline FROM ")) die_nomem();

    if (!stralloc_cats(&line,table)) die_nomem();
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
    result = PQexec(psql,line.s);
    if (result == NULL)
      strerr_die2x(111,FATAL,PQerrorMessage(psql));
    if (PQresultStatus(result) != PGRES_TUPLES_OK)
      strerr_die2x(111,FATAL,PQresultErrorMessage(result));
    
    for(row_nr=0; row_nr<PQntuples(result); row_nr++) {
      row = PQgetvalue(result,row_nr,0);
      length = PQgetlength(result,row_nr,0);
      if (subwrite(row,length) == -1) die_write();
    }
    PQclear(result);
	
  }
}
