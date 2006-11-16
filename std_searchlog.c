/*$Id$*/

#include <unistd.h>
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
#include "sub_std.h"
#include "die.h"

static stralloc line = {0};
static stralloc outline = {0};
static char date[DATE822FMT];
static datetime_sec when;
static substdio ssin;
static char inbuf[256];

static void lineout(int subwrite())
{
  struct datetime dt;
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

void std_searchlog(const char *dir,		/* work directory */
		   const char *subdir,
		   char *search,		/* search string */
		   int subwrite())		/* output fxn */
/* opens dir/Log, and outputs via subwrite(s,len) any line that matches */
/* search. A '_' is search is a wildcard. Any other non-alphanum/'.' char */
/* is replaced by a '_' */
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

  searchlen = str_len(search);
  case_lowerb(search,searchlen);
  cps = (unsigned char *) search;
  while ((ch = *(cps++))) {		/* search is potentially hostile */
    if (ch >= 'a' && ch <= 'z') continue;
    if (ch >= '0' && ch <= '9') continue;
    if (ch == '.' || ch == '_') continue;
    *(cps - 1) = '_';			/* will [also] match char specified */
  }

  std_makepath(&line,dir,subdir,"/Log",0);
  fd = open_read(line.s);
  if (fd == -1) {
    if (errno != error_noent)
      strerr_die4sys(111,FATAL,ERR_OPEN,line.s,": ");
    else
      strerr_die3x(100,FATAL,line.s,ERR_NOEXIST);
  }
  substdio_fdbuf(&ssin,read,fd,inbuf,sizeof(inbuf));

  for (;;) {
    if (getln(&ssin,&line,&match,'\n') == -1)
      strerr_die2sys(111,FATAL,ERR_READ_INPUT);
    if (!match) break;
    if (!searchlen) {
      lineout(subwrite);
    } else {		/* simple case-insensitive search */
      cpline = (unsigned char *) line.s - 1;
      cplast = cpline + line.len - searchlen; /* line has \0 at the end */
      while ((cp = ++cpline) <= cplast) {
	cpsearch = (unsigned char *) search;
	for (;;) {
	  x = *cpsearch++;
	  if (!x) break;
	  y = *cp++ - 'A';
	  if (y <= 'Z' - 'A') y += 'a'; else y += 'A';
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
}
