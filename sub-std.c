#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "byte.h"
#include "case.h"
#include "config.h"
#include "cookie.h"
#include "date822fmt.h"
#include "datetime.h"
#include "die.h"
#include "error.h"
#include "msgtxt.h"
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
#include "sub_std.h"
#include "subhash.h"
#include "substdio.h"
#include "subdb.h"
#include "uint32.h"
#include "wrap.h"

static stralloc addr = {0};
static stralloc lcaddr = {0};
static stralloc fn = {0};
static stralloc fnnew = {0};
static stralloc fnlock = {0};
static stralloc outline = {0};

static char ssbuf[512];
static char inbuf[512];
static char ssnewbuf[256];

static datetime_sec when;

static substdio ss;
static substdio ssin;
static substdio ssnew;

static int fd;
static int fdnew;

static void die_read(void)
{
  strerr_die2sys(111,FATAL,MSG1("ERR_READ",fn.s));
}

static void die_write(const char *name)
{
  strerr_die4sys(111,FATAL,MSG("ERR_WRITE"),name,": ");
}

static void _closesub(struct subdbinfo *info)
{
  (void)info;
}

static const char *_opensub(struct subdbinfo *info)
{
  msgtxt_init();
  return 0;
  (void)info;
}

/* Checks that the hash of seed matches hash and returns success
 * (NULL). If not match returns "". If error, returns error string */
static const char *_checktag(struct subdbinfo *info,
			     unsigned long num,	/* message number */
			     unsigned long listno,
			     const char *action,
			     const char *seed,	/* cookie base */
			     const char *hash)	/* cookie */
{
  char strnum[FMT_ULONG];			/* message number as sz */
  char newcookie[COOKIE];

  if (!seed) return (char *) 0;			/* no data - accept */

  strnum[fmt_ulong(strnum,num)] = '\0';		/* message nr ->string*/

  cookie(newcookie,key.s,key.len,strnum,seed,action);
  if (byte_diff(hash,COOKIE,newcookie)) return "";
  else return (char *) 0;

  (void)info;
  (void)listno;
}

static int _issub(struct subdbinfo *info,
		  const char *subdir,
		  const char *userhost,
		  stralloc *recorded)
{
  static stralloc line = {0};

  int fd;
  unsigned int j;
  char ch,lcch;
  int match;

    if (!stralloc_copys(&addr,"T")) die_nomem();
    if (!stralloc_cats(&addr,userhost)) die_nomem();

    j = byte_rchr(addr.s,addr.len,'@');
    if (j == addr.len) return 0;
    case_lowerb(addr.s + j + 1,addr.len - j - 1);
    if (!stralloc_copy(&lcaddr,&addr)) die_nomem();
    case_lowerb(lcaddr.s + 1,j - 1);	/* totally lc version of addr */

    /* make hash for both for backwards comp */
    ch = 64 + subhashsa(&addr);
    lcch = 64 + subhashsa(&lcaddr);

    if (!stralloc_0(&addr)) die_nomem();
    if (!stralloc_0(&lcaddr)) die_nomem();
    makepath(&fn,subdir,"/subscribers/",lcch);

    fd = open_read(fn.s);
    if (fd == -1) {
      if (errno != error_noent)
        strerr_die2sys(111,FATAL,MSG1("ERR_OPEN",fn.s));
    } else {
      substdio_fdbuf(&ss,read,fd,ssbuf,sizeof(ssbuf));

      for (;;) {
        if (getln(&ss,&line,&match,'\0') == -1)
          strerr_die2sys(111,FATAL,MSG1("ERR_READ",fn.s));
        if (!match) break;
        if (line.len == lcaddr.len)
          if (!case_diffb(line.s,line.len,lcaddr.s)) {
	    close(fd);
	    if (recorded)
	      if (!stralloc_copyb(recorded,line.s+1,line.len-1))
		strerr_die2x(111,FATAL,MSG("ERR_NOMEM"));
	    return 1;
	  }
      }

      close(fd);
    }
	/* here if file not found or (file found && addr not there) */

    if (ch == lcch) return 0;

	/* try case sensitive hash for backwards compatibility */
    fn.s[fn.len - 2] = ch;
    fd = open_read(fn.s);
    if (fd == -1) {
      if (errno != error_noent)
        strerr_die2sys(111,FATAL,MSG1("ERR_OPEN",fn.s));
      return 0;
    }
    substdio_fdbuf(&ss,read,fd,ssbuf,sizeof(ssbuf));

    for (;;) {
      if (getln(&ss,&line,&match,'\0') == -1)
        strerr_die2sys(111,FATAL,MSG1("ERR_READ",fn.s));
      if (!match) break;
      if (line.len == addr.len)
        if (!case_diffb(line.s,line.len,addr.s)) {
	  close(fd);
	  if (recorded)
	    if (!stralloc_copyb(recorded,line.s+1,line.len-1))
	      strerr_die2x(111,FATAL,MSG("ERR_NOMEM"));
	  return 1;
	}
    }

    close(fd);

    return 0;

    (void)info;
}

static const char *_logmsg(struct subdbinfo *info,
			   unsigned long msgnum,
			   unsigned long listno,
			   unsigned long subs,
			   int done)
{
  return 0;
  (void)info;
  (void)msgnum;
  (void)listno;
  (void)subs;
  (void)done;
}

/* Outputs all userhostesses in 'subdir' through subwrite. 'subdir' is
 * the subdirectory name.  subwrite must be a function returning >=0 on
 * success, -1 on error, and taking arguments (char* string, unsigned
 * int length). It will be called once per address and should take care
 * of newline or whatever needed for the output form. */
static unsigned long _putsubs(struct subdbinfo *info,
			      const char *subdir,
			      unsigned long hash_lo,
			      unsigned long hash_hi,
			      int subwrite())	/* write function. */
{
  static stralloc line = {0};

  unsigned int i;
  int fd;
  unsigned long no = 0L;
  int match;
  unsigned int hashpos;

    makepath(&fn,subdir,"/subscribers/",'?');
    hashpos = fn.len - 2;
    if (hash_lo > 52) hash_lo = 52;
    if (hash_hi > 52) hash_hi = 52;
    if (hash_hi < hash_lo) hash_hi = hash_lo;

    for (i = hash_lo;i <= hash_hi;++i) {
      fn.s[hashpos] = 64 + i;	/* hash range 0-52 */
      fd = open_read(fn.s);
      if (fd == -1) {
        if (errno != error_noent)
	  strerr_die2sys(111,FATAL,MSG1("ERR_READ",fn.s));
      } else {
        substdio_fdbuf(&ssin,read,fd,inbuf,sizeof(inbuf));
        for (;;) {
          if (getln(&ssin,&line,&match,'\0') == -1)
            strerr_die2sys(111,FATAL,MSG1("ERR_READ",fn.s));
          if (!match)
            break;
          if (subwrite(line.s + 1,line.len - 2) == -1) die_write("stdout");
          no++;
        }
        close(fd);
      }
    }
    return no;

    (void)info;
}

static void lineout(const stralloc *line, int subwrite())
{
  struct datetime dt;
  char date[DATE822FMT];

  (void) scan_ulong(line->s,&when);
  datetime_tai(&dt,when);		/* there is always at least a '\n' */
  if (!stralloc_copyb(&outline,date,date822fmt(date,&dt) - 1))
	die_nomem();
  if (!stralloc_cats(&outline,": ")) die_nomem();
  if (!stralloc_catb(&outline,line->s,line->len - 1)) die_nomem();
  if (subwrite(outline.s,outline.len) == -1)
	strerr_die3x(111,FATAL,MSG("ERR_WRITE"),"output");
  return;
}

/* opens dir/Log, and outputs via subwrite(s,len) any line that matches
 * search. A '_' is search is a wildcard. Any other non-alphanum/'.'
 * char is replaced by a '_' */
static void _searchlog(struct subdbinfo *info,
		       const char *subdir,
		       char *search,		/* search string */
		       int subwrite())		/* output fxn */
{
  static stralloc line = {0};

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

  makepath(&line,subdir,"/Log",0);
  fd = open_read(line.s);
  if (fd == -1) {
    if (errno != error_noent)
      strerr_die2sys(111,FATAL,MSG1("ERR_OPEN",line.s));
    else
      strerr_die3x(100,FATAL,line.s,MSG("ERR_NOEXIST"));
  }
  substdio_fdbuf(&ssin,read,fd,inbuf,sizeof(inbuf));

  for (;;) {
    if (getln(&ssin,&line,&match,'\n') == -1)
      strerr_die2sys(111,FATAL,MSG("ERR_READ_INPUT"));
    if (!match) break;
    if (!searchlen) {
      lineout(&line,subwrite);
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
	  lineout(&line,subwrite);
	  break;
	}
      }
    }
  }
  close(fd);

  (void)info;
}

/* Add (flagadd=1) or remove (flagadd=0) userhost from the
 * subscr. Comment is e.g. the subscriber from line or name. It is added
 * to the log. Event is the action type, e.g. "probe", "manual",
 * etc. The direction (sub/unsub) is inferred from flagadd. Returns 1 on
 * success, 0 on failure. If forcehash is >=0 it is used in place of the
 * calculated hash. This makes it possible to add addresses with a hash
 * that does not exist. forcehash has to be 0..99.  for unsubscribes,
 * the address is only removed if forcehash matches the actual
 * hash. This way, ezmlm-manage can be prevented from touching certain
 * addresses that can only be removed by ezmlm-unsub. Usually, this
 * would be used for sublist addresses (to avoid removal) and sublist
 * aliases (to prevent users from subscribing them (although the cookie
 * mechanism would prevent the resulting duplicate message from being
 * distributed. */
static int _subscribe(struct subdbinfo *info,
		      const char *subdir,
		      const char *userhost,
		      int flagadd,
		      const char *comment,
		      const char *event,
		      int forcehash)
{
  static stralloc line = {0};

  int fdlock;

  unsigned int j;
  unsigned char ch,lcch;
  int match;
  int flagwasthere;

  if (userhost[str_chr(userhost,'\n')])
    strerr_die2x(100,FATAL,MSG("ERR_ADDR_NL"));

    if (!stralloc_copys(&addr,"T")) die_nomem();
    if (!stralloc_cats(&addr,userhost)) die_nomem();
    if (addr.len > 401)
      strerr_die2x(100,FATAL,MSG("ERR_ADDR_LONG"));

    j = byte_rchr(addr.s,addr.len,'@');
    if (j == addr.len)
      strerr_die2x(100,FATAL,MSG("ERR_ADDR_AT"));
    case_lowerb(addr.s + j + 1,addr.len - j - 1);
    if (!stralloc_copy(&lcaddr,&addr)) die_nomem();
    case_lowerb(lcaddr.s + 1,j - 1);	/* make all-lc version of address */

    if (forcehash >= 0 && forcehash <= 52) {
      ch = lcch = 64 + (unsigned char) forcehash;
    } else {
      ch = 64 + subhashsa(&addr);
      lcch = 64 + subhashsa(&lcaddr);
    }

    if (!stralloc_0(&addr)) die_nomem();
    if (!stralloc_0(&lcaddr)) die_nomem();
    makepath(&fn,subdir,"/subscribers/",lcch);
    makepath(&fnlock,subdir,"/lock",0);

    if (!stralloc_copyb(&fnnew,fn.s,fn.len-1)) die_nomem();
	/* code later depends on fnnew = fn + 'n' */
    if (!stralloc_cats(&fnnew,"n")) die_nomem();
    if (!stralloc_0(&fnnew)) die_nomem();

    fdlock = lockfile(fnlock.s);

				/* do lower case hashed version first */
    fdnew = open_trunc(fnnew.s);
    if (fdnew == -1) die_write(fnnew.s);
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
	  close(fd); close(fdnew); die_write(fnnew.s);
        }
      }

      close(fd);
    }

    if (flagadd && !flagwasthere)
      if (substdio_bput(&ssnew,addr.s,addr.len) == -1) {
        close(fdnew); die_write(fnnew.s);
      }

    if (substdio_flush(&ssnew) == -1) { close(fdnew); die_write(fnnew.s); }
    if (fsync(fdnew) == -1) { close(fdnew); die_write(fnnew.s); }
    close(fdnew);

    if (rename(fnnew.s,fn.s) == -1)
      strerr_die6sys(111,FATAL,MSG("ERR_MOVE"),fnnew.s," to ",fn.s,": ");

    if ((ch == lcch) || flagwasthere) {
      close(fdlock);
      if (flagadd ^ flagwasthere) {
        if (!stralloc_0(&addr)) die_nomem();
        logaddr(subdir,event,addr.s+1,comment);
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
    if (fdnew == -1) die_write(fnnew.s);
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
          { close(fd); close(fdnew); die_write(fnnew.s); }
      }

      close(fd);
    }

    if (substdio_flush(&ssnew) == -1) { close(fdnew); die_write(fnnew.s); }
    if (fsync(fdnew) == -1) { close(fdnew); die_write(fnnew.s); }
    close(fdnew);

    if (rename(fnnew.s,fn.s) == -1)
      strerr_die6sys(111,FATAL,MSG("ERR_MOVE"),fnnew.s," to ",fn.s,": ");

    close(fdlock);
    if (flagadd ^ flagwasthere) {
      if (!stralloc_0(&addr)) die_nomem();
      logaddr(subdir,event,addr.s+1,comment);
      return 1;
    }
    return 0;

    (void)info;
}

static void _tagmsg(struct subdbinfo *info,
		    unsigned long msgnum,
		    const char *hashout,
		    unsigned long bodysize,
		    unsigned long chunk)
{
  (void)info;
  (void)msgnum;
  (void)hashout;
  (void)bodysize;
  (void)chunk;
}

static int isdir(const char *path)
{
  struct stat st;
  return wrap_stat(path, &st) == 0
    && S_ISDIR(st.st_mode);
}

static const char *_mktab(struct subdbinfo *info)
{
  if (mkdir("subscribers", 0777) < 0
      && errno != error_exist)
    return strerror(errno);
  if (isdir("allow"))
    if (mkdir("allow/subscribers", 0777) < 0
	&& errno != error_exist)
      return strerror(errno);
  if (isdir("deny"))
    if (mkdir("deny/subscribers", 0777) < 0
	&& errno != error_exist)
      return strerror(errno);
  if (isdir("digest"))
    if (mkdir("digest/subscribers", 0777) < 0
	&& errno != error_exist)
      return strerror(errno);
  if (isdir("mod"))
    if (mkdir("mod/subscribers", 0777) < 0
	&& errno != error_exist)
      return strerror(errno);
  return 0;
  (void)info;
}

static const char *rmsubs(const char *subdir)
{
  int ch;
  for (ch = 0; ch < 53; ++ch) {
    makepath(&fn,subdir,"/subscribers/",ch + 64);
    unlink(fn.s);
  }
  makepath(&fn,subdir,"/subscribers",0);
  if (rmdir(fn.s) < 0
      && errno != error_noent)
    return strerror(errno);
  return 0;
}

static const char *_rmtab(struct subdbinfo *info)
{
  const char *r;
  if ((r = rmsubs(".")) != 0) return r;
  if ((r = rmsubs("allow")) != 0) return r;
  if ((r = rmsubs("deny")) != 0) return r;
  if ((r = rmsubs("digest")) != 0) return r;
  if ((r = rmsubs("mod")) != 0) return r;
  return 0;
  (void)info;
}

struct sub_plugin sub_plugin = {
  SUB_PLUGIN_VERSION,
  _checktag,
  _closesub,
  _issub,
  _logmsg,
  _mktab,
  _opensub,
  _putsubs,
  _rmtab,
  _searchlog,
  _subscribe,
  _tagmsg,
};
