/*$Id$*/
#include "stralloc.h"
#include "getln.h"
#include "readwrite.h"
#include "substdio.h"
#include "open.h"
#include "byte.h"
#include "case.h"
#include "strerr.h"
#include "error.h"
#include "uint32.h"
#include "fmt.h"
#include "subscribe.h"
#include "errtxt.h"

static void die_nomem(fatal)
char *fatal;
{
  strerr_die2x(111,fatal,ERR_NOMEM);
}

static stralloc addr = {0};
static stralloc lcaddr = {0};
static stralloc line = {0};
static stralloc quoted = {0};
static stralloc fn = {0};
static substdio ss;
static char ssbuf[512];
static char szh[FMT_ULONG];

char *issub(dbname,userhost,tab,fatal)
/* Returns (char *) to match if userhost is in the subscriber database     */
/* dbname, 0 otherwise. dbname is a base directory for a list and may NOT  */
/* be NULL        */
/* NOTE: The returned pointer is NOT VALID after a subsequent call to issub!*/

char *dbname;		/* directory to basedir */
char *userhost;
char *tab;		/* override table name */
char *fatal;

{

  int fd;
  unsigned int j;
  uint32 h,lch;
  char ch,lcch;
  int match;

    if (!stralloc_copys(&addr,"T")) die_nomem(fatal);
    if (!stralloc_cats(&addr,userhost)) die_nomem(fatal);

    j = byte_rchr(addr.s,addr.len,'@');
    if (j == addr.len) return (char *) 0;
    case_lowerb(addr.s + j + 1,addr.len - j - 1);
    if (!stralloc_copy(&lcaddr,&addr)) die_nomem(fatal);
    case_lowerb(lcaddr.s + 1,j - 1);	/* totally lc version of addr */

    h = 5381;
    lch = h;			/* make hash for both for backwards comp */
    for (j = 0;j < addr.len;++j) {	/* (lcaddr.len == addr.len) */
      h = (h + (h << 5)) ^ (uint32) (unsigned char) addr.s[j];
      lch = (lch + (lch << 5)) ^ (uint32) (unsigned char) lcaddr.s[j];
    }
    ch = 64 + (h % 53);
    lcch = 64 + (lch % 53);

    if (!stralloc_0(&addr)) die_nomem(fatal);
    if (!stralloc_0(&lcaddr)) die_nomem(fatal);
    if (!stralloc_copys(&fn,dbname)) die_nomem(fatal);
    if (!stralloc_cats(&fn,"/subscribers/")) die_nomem(fatal);
    if (!stralloc_catb(&fn,&lcch,1)) die_nomem(fatal);
    if (!stralloc_0(&fn)) die_nomem(fatal);

    fd = open_read(fn.s);
    if (fd == -1) {
      if (errno != error_noent)
        strerr_die4sys(111,fatal,ERR_OPEN,fn.s,": ");
    } else {
      substdio_fdbuf(&ss,read,fd,ssbuf,sizeof(ssbuf));

      for (;;) {
        if (getln(&ss,&line,&match,'\0') == -1)
          strerr_die4sys(111,fatal,ERR_READ,fn.s,": ");
        if (!match) break;
        if (line.len == lcaddr.len)
          if (!case_diffb(line.s,line.len,lcaddr.s))
            { close(fd); return line.s+1; }
      }

      close(fd);
    }
	/* here if file not found or (file found && addr not there) */

    if (ch == lcch) return (char *) 0;

	/* try case sensitive hash for backwards compatibility */
    fn.s[fn.len - 2] = ch;
    fd = open_read(fn.s);
    if (fd == -1) {
      if (errno != error_noent)
        strerr_die4sys(111,fatal,ERR_OPEN,fn.s,": ");
      return (char *) 0;
    }
    substdio_fdbuf(&ss,read,fd,ssbuf,sizeof(ssbuf));

    for (;;) {
      if (getln(&ss,&line,&match,'\0') == -1)
        strerr_die4sys(111,fatal,ERR_READ,fn.s,": ");
      if (!match) break;
      if (line.len == addr.len)
        if (!case_diffb(line.s,line.len,addr.s))
          { close(fd); return line.s+1; }
    }

    close(fd);

    return (char *) 0;
}
