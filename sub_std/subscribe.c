/*$Id$*/

#include <stdio.h>
#include <unistd.h>
#include "str.h"
#include "stralloc.h"
#include "getln.h"
#include "readwrite.h"
#include "substdio.h"
#include "strerr.h"
#include "open.h"
#include "byte.h"
#include "case.h"
#include "lock.h"
#include "error.h"
#include "subscribe.h"
#include "uint32.h"
#include "fmt.h"
#include "errtxt.h"
#include "log.h"
#include "die.h"

static stralloc addr = {0};
static stralloc lcaddr = {0};
static stralloc line = {0};
static stralloc fnnew = {0};
static stralloc fn = {0};
static stralloc fnlock = {0};

void die_read(void)
{
  strerr_die4sys(111,FATAL,ERR_READ,fn.s,": ");
}

void die_write(void)
{
  strerr_die4sys(111,FATAL,ERR_WRITE,fnnew.s,": ");
}

static int fd;
static substdio ss;
static char ssbuf[256];
static int fdnew;
static substdio ssnew;
static char ssnewbuf[256];

int subscribe(const char *dbname,
	      const char *userhost,
	      int flagadd,
	      const char *comment,
	      const char *event,
	      int flagmysql,
	      int forcehash,
	      const char *tab)
/* add (flagadd=1) or remove (flagadd=0) userhost from the subscr. database  */
/* dbname. Comment is e.g. the subscriber from line or name. It is added to  */
/* the log. Event is the action type, e.g. "probe", "manual", etc. The       */
/* direction (sub/unsub) is inferred from flagadd. Returns 1 on success, 0   */
/* on failure. If flagmysql is set and the file "sql" is found in the        */
/* directory dbname, it is parsed and a mysql db is assumed. if forcehash is */
/* >=0 it is used in place of the calculated hash. This makes it possible to */
/* add addresses with a hash that does not exist. forcehash has to be 0..99. */
/* for unsubscribes, the address is only removed if forcehash matches the    */
/* actual hash. This way, ezmlm-manage can be prevented from touching certain*/
/* addresses that can only be removed by ezmlm-unsub. Usually, this would be */
/* used for sublist addresses (to avoid removal) and sublist aliases (to     */
/* prevent users from subscribing them (although the cookie mechanism would  */
/* prevent the resulting duplicate message from being distributed. */
{
  int fdlock;

  unsigned int j;
  uint32 h,lch;
  unsigned char ch,lcch;
  int match;
  int flagwasthere;

  if (userhost[str_chr(userhost,'\n')])
    strerr_die2x(100,FATAL,ERR_ADDR_NL);

    if (!stralloc_copys(&addr,"T")) die_nomem();
    if (!stralloc_cats(&addr,userhost)) die_nomem();
    if (addr.len > 401)
      strerr_die2x(100,FATAL,ERR_ADDR_LONG);

    j = byte_rchr(addr.s,addr.len,'@');
    if (j == addr.len)
      strerr_die2x(100,FATAL,ERR_ADDR_AT);
    case_lowerb(addr.s + j + 1,addr.len - j - 1);
    if (!stralloc_copy(&lcaddr,&addr)) die_nomem();
    case_lowerb(lcaddr.s + 1,j - 1);	/* make all-lc version of address */

    if (forcehash >= 0 && forcehash <= 52) {
      ch = lcch = (unsigned char) forcehash;
    } else {
      h = 5381;
      lch = h;
      for (j = 0;j < addr.len;++j) {
        h = (h + (h << 5)) ^ (uint32) (unsigned char) addr.s[j];
        lch = (lch + (lch << 5)) ^ (uint32) (unsigned char) lcaddr.s[j];
      }
      lcch = 64 + (lch % 53);
      ch = 64 + (h % 53);
    }

    if (!stralloc_0(&addr)) die_nomem();
    if (!stralloc_0(&lcaddr)) die_nomem();
    if (!stralloc_copys(&fn,dbname)) die_nomem();
    if (!stralloc_copys(&fnlock,dbname)) die_nomem();

    if (!stralloc_cats(&fn,"/subscribers/")) die_nomem();
    if (!stralloc_catb(&fn,&lcch,1)) die_nomem();
    if (!stralloc_copy(&fnnew,&fn)) die_nomem();
	/* code later depends on fnnew = fn + 'n' */
    if (!stralloc_cats(&fnnew,"n")) die_nomem();
    if (!stralloc_cats(&fnlock,"/lock")) die_nomem();
    if (!stralloc_0(&fnnew)) die_nomem();
    if (!stralloc_0(&fn)) die_nomem();
    if (!stralloc_0(&fnlock)) die_nomem();

    fdlock = lockfile(fnlock.s);

				/* do lower case hashed version first */
    fdnew = open_trunc(fnnew.s);
    if (fdnew == -1) die_write();
    substdio_fdbuf(&ssnew,write,fdnew,ssnewbuf,sizeof(ssnewbuf));

    flagwasthere = 0;

    fd = open_read(fn.s);
    if (fd == -1) {
      if (errno != error_noent) { close(fdnew); die_read(); }
    }
    else {
      substdio_fdbuf(&ss,read,fd,ssbuf,sizeof(ssbuf));

      for (;;) {
        if (getln(&ss,&line,&match,'\0') == -1) {
	  close(fd); close(fdnew); die_read();
        }
        if (!match) break;
        if (line.len == addr.len)
          if (!case_diffb(line.s,line.len,addr.s)) {
	    flagwasthere = 1;
	    if (!flagadd)
	      continue;
	  }
        if (substdio_bput(&ssnew,line.s,line.len) == -1) {
	  close(fd); close(fdnew); die_write();
        }
      }

      close(fd);
    }

    if (flagadd && !flagwasthere)
      if (substdio_bput(&ssnew,addr.s,addr.len) == -1) {
        close(fdnew); die_write();
      }

    if (substdio_flush(&ssnew) == -1) { close(fdnew); die_write(); }
    if (fsync(fdnew) == -1) { close(fdnew); die_write(); }
    close(fdnew);

    if (rename(fnnew.s,fn.s) == -1)
      strerr_die6sys(111,FATAL,ERR_MOVE,fnnew.s," to ",fn.s,": ");

    if ((ch == lcch) || flagwasthere) {
      close(fdlock);
      if (flagadd ^ flagwasthere) {
        if (!stralloc_0(&addr)) die_nomem();
        logaddr(dbname,event,addr.s+1,comment);
        return 1;
      }
      return 0;
    }

			/* If unsub and not found and hashed differ, OR */
			/* sub and not found (so added with new hash) */
			/* do the 'case-dependent' hash */

    fn.s[fn.len - 2] = ch;
    fnnew.s[fnnew.len - 3] = ch;
    fdnew = open_trunc(fnnew.s);
    if (fdnew == -1) die_write();
    substdio_fdbuf(&ssnew,write,fdnew,ssnewbuf,sizeof(ssnewbuf));

    fd = open_read(fn.s);
    if (fd == -1) {
      if (errno != error_noent) { close(fdnew); die_read(); }
    } else {
      substdio_fdbuf(&ss,read,fd,ssbuf,sizeof(ssbuf));

      for (;;) {
        if (getln(&ss,&line,&match,'\0') == -1)
          { close(fd); close(fdnew); die_read(); }
        if (!match) break;
        if (line.len == addr.len)
          if (!case_diffb(line.s,line.len,addr.s)) {
            flagwasthere = 1;
            continue;	/* always want to remove from case-sensitive hash */
          }
        if (substdio_bput(&ssnew,line.s,line.len) == -1)
          { close(fd); close(fdnew); die_write(); }
      }

      close(fd);
    }

    if (substdio_flush(&ssnew) == -1) { close(fdnew); die_write(); }
    if (fsync(fdnew) == -1) { close(fdnew); die_write(); }
    close(fdnew);

    if (rename(fnnew.s,fn.s) == -1)
      strerr_die6sys(111,FATAL,ERR_MOVE,fnnew.s," to ",fn.s,": ");

    close(fdlock);
    if (flagadd ^ flagwasthere) {
      if (!stralloc_0(&addr)) die_nomem();
      logaddr(dbname,event,addr.s+1,comment);
      return 1;
    }
    return 0;

}
