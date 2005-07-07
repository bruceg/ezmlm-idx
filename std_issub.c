/*$Id$*/

#include <unistd.h>
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
#include "subhash.h"
#include "fmt.h"
#include "subscribe.h"
#include "errtxt.h"
#include "die.h"
#include "idx.h"

static stralloc addr = {0};
static stralloc lcaddr = {0};
static stralloc line = {0};
static stralloc fn = {0};
static substdio ss;
static char ssbuf[512];

const char *std_issub(const char *dbname,	/* directory to basedir */
		      const char *userhost)
/* Returns (char *) to match if userhost is in the subscriber database     */
/* dbname, 0 otherwise. dbname is a base directory for a list and may NOT  */
/* be NULL        */
/* NOTE: The returned pointer is NOT VALID after a subsequent call to issub!*/
{

  int fd;
  unsigned int j;
  char ch,lcch;
  int match;

    if (!stralloc_copys(&addr,"T")) die_nomem();
    if (!stralloc_cats(&addr,userhost)) die_nomem();

    j = byte_rchr(addr.s,addr.len,'@');
    if (j == addr.len) return (char *) 0;
    case_lowerb(addr.s + j + 1,addr.len - j - 1);
    if (!stralloc_copy(&lcaddr,&addr)) die_nomem();
    case_lowerb(lcaddr.s + 1,j - 1);	/* totally lc version of addr */

    /* make hash for both for backwards comp */
    ch = 64 + subhashsa(&addr);
    lcch = 64 + subhashsa(&lcaddr);

    if (!stralloc_0(&addr)) die_nomem();
    if (!stralloc_0(&lcaddr)) die_nomem();
    if (!stralloc_copys(&fn,dbname)) die_nomem();
    if (!stralloc_cats(&fn,"/subscribers/")) die_nomem();
    if (!stralloc_catb(&fn,&lcch,1)) die_nomem();
    if (!stralloc_0(&fn)) die_nomem();

    fd = open_read(fn.s);
    if (fd == -1) {
      if (errno != error_noent)
        strerr_die4sys(111,FATAL,ERR_OPEN,fn.s,": ");
    } else {
      substdio_fdbuf(&ss,read,fd,ssbuf,sizeof(ssbuf));

      for (;;) {
        if (getln(&ss,&line,&match,'\0') == -1)
          strerr_die4sys(111,FATAL,ERR_READ,fn.s,": ");
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
        strerr_die4sys(111,FATAL,ERR_OPEN,fn.s,": ");
      return (char *) 0;
    }
    substdio_fdbuf(&ss,read,fd,ssbuf,sizeof(ssbuf));

    for (;;) {
      if (getln(&ss,&line,&match,'\0') == -1)
        strerr_die4sys(111,FATAL,ERR_READ,fn.s,": ");
      if (!match) break;
      if (line.len == addr.len)
        if (!case_diffb(line.s,line.len,addr.s))
          { close(fd); return line.s+1; }
    }

    close(fd);

    return (char *) 0;
}
