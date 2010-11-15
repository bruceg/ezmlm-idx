#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <unistd.h>
#include "direntry.h"
#include "readwrite.h"
#include "getln.h"
#include "substdio.h"
#include "stralloc.h"
#include "slurp.h"
#include "getconfopt.h"
#include "getconf.h"
#include "byte.h"
#include "error.h"
#include "str.h"
#include "strerr.h"
#include "sig.h"
#include "now.h"
#include "datetime.h"
#include "fmt.h"
#include "cookie.h"
#include "qmail.h"
#include "messages.h"
#include "quote.h"
#include "open.h"
#include "scan.h"
#include "lock.h"
#include "copy.h"
#include "mime.h"
#include "hdr.h"
#include "die.h"
#include "wrap.h"
#include "idx.h"
#include "config.h"
#include "subdb.h"

const char FATAL[] = "ezmlm-warn: fatal: ";
const char USAGE[] =
"ezmlm-warn: usage: ezmlm-warn -dD -l secs -t days dir";

static int flagdig = -1;
static unsigned long lockout = 0L;
static unsigned long bouncetimeout = BOUNCE_TIMEOUT;
static int nowarn = 0;
unsigned long copylines = 0;	/* Number of lines from the message to copy */
static struct option options[] = {
  OPT_FLAG(flagdig,'d',1,0),
  OPT_FLAG(flagdig,'D',0,0),
  OPT_ULONG(lockout,'l',0),
  OPT_ULONG(bouncetimeout,'t',0),
  OPT_ULONG(copylines,0,"copylines"),
  OPT_FLAG(nowarn,0,1,"nowarn"),
  OPT_END
};

char boundary[COOKIE];

substdio ssout;
char outbuf[16];

unsigned long when;
const char *workdir;
stralloc fn = {0};
stralloc bdname = {0};
stralloc fnlasth = {0};
stralloc fnlastd = {0};
stralloc lasth = {0};
stralloc lastd = {0};
struct stat st;

static void die_read(void) { strerr_die2sys(111,FATAL,MSG1(ERR_READ,fn.s)); }

void makedir(const char *s)
{
  if (mkdir(s,0755) == -1)
    if (errno != error_exist)
      strerr_die2sys(111,FATAL,MSG1(ERR_CREATE,s));
}

char inbuf[1024];
substdio ssin;
char textbuf[1024];
substdio sstext;

stralloc addr = {0};
char strnum[FMT_ULONG];
char hash[COOKIE];
stralloc fnhash = {0};
stralloc quoted = {0};
stralloc line = {0};
stralloc qline = {0};

struct qmail qq;

static void code_qput(const char *s,unsigned int n)
{
    if (!flagcd)
      qmail_put(&qq,s,n);
    else {
      if (flagcd == 'B')
        encodeB(s,n,&qline,0);
      else
        encodeQ(s,n,&qline);
      qmail_put(&qq,qline.s,qline.len);
    }
}

static void doit(int flagdig, int flagw)
{
  unsigned int i;
  int fd;
  int match;
  int fdhash;
  const char *err;

  fd = open_read(fn.s);
  if (fd == -1) die_read();
  substdio_fdbuf(&ssin,read,fd,inbuf,sizeof(inbuf));

  if (getln(&ssin,&addr,&match,'\0') == -1) die_read();
  if (!match) { close(fd); return; }
  if (!issub(workdir,addr.s,0)) { close(fd); /*XXX*/unlink(fn.s); return; }
  cookie(hash,"",0,"",addr.s,"");
  if (!stralloc_copys(&fnhash,workdir)) die_nomem();
  if (!stralloc_cats(&fnhash,"/bounce/h/")) die_nomem();
  if (!stralloc_catb(&fnhash,hash,1)) die_nomem();
  if (!stralloc_cats(&fnhash,"/h")) die_nomem();
  if (!stralloc_catb(&fnhash,hash+1,COOKIE-1)) die_nomem();
  if (!stralloc_0(&fnhash)) die_nomem();

  if (qmail_open(&qq) == -1)
    strerr_die2sys(111,FATAL,MSG(ERR_QMAIL_QUEUE));

  hdr_add2s("Mailing-List: ",MSG(TXT_MAILING_LIST));
  if (listid.len > 0)
    hdr_add2("\nList-ID: ",listid.s,listid.len);
  hdr_datemsgid(now());
  if (flagcd) {
    if (!stralloc_0(&line)) die_nomem();
  }
  hdr_from("-help");
  if (!quote2(&quoted,addr.s)) die_nomem();
  hdr_add2("To: ",quoted.s,quoted.len);
  /* to accomodate transfer-encoding */
  hdr_mime(flagcd ? CTYPE_MULTIPART : CTYPE_TEXT);
  hdr_subject(flagw ? MSG(SUB_PROBE) : MSG(SUB_WARNING));

  if (flagcd) {			/* first part for QP/base64 multipart msg */
    hdr_boundary(0);
    hdr_ctype(CTYPE_TEXT);
    hdr_transferenc();
  } else
    qmail_puts(&qq,"\n");

  copy(&qq,"text/top",flagcd);
  copy(&qq,flagw ? "text/bounce-probe" : "text/bounce-warn",flagcd);

  if (!flagw) {
    if (flagdig)
      copy(&qq,"text/dig-bounce-num",flagcd);
    else
      copy(&qq,"text/bounce-num",flagcd);
    if (!flagcd) {
      fdhash = open_read(fnhash.s);
      if (fdhash == -1) {
        if (errno != error_noent)
          strerr_die2sys(111,FATAL,MSG1(ERR_OPEN,fnhash.s));
      } else {
        substdio_fdbuf(&sstext,read,fdhash,textbuf,sizeof(textbuf));
        for(;;) {
          if (getln(&sstext,&line,&match,'\n') == -1)
            strerr_die2sys(111,FATAL,MSG1(ERR_READ,fnhash.s));
          if (!match) break;
          code_qput(line.s,line.len);
        }
      }
      close(fdhash);
    } else {
      if (!stralloc_copys(&line,"")) die_nomem();	/* slurp adds! */
      if (slurp(fnhash.s,&line,256) < 0)
        strerr_die2sys(111,FATAL,MSG1(ERR_OPEN,fnhash.s));
      code_qput(line.s,line.len);
    }
  }

  copy(&qq,"text/bounce-bottom",flagcd);
  if (flagcd) {
    if (flagcd == 'B') {
      encodeB("",0,&line,2);
      qmail_put(&qq,line.s,line.len);	/* flush */
    }
    hdr_boundary(0);
    hdr_ctype(CTYPE_MESSAGE);
    qmail_puts(&qq,"\n");
  }
  if (qmail_copy(&qq,&ssin,copylines) < 0) die_read();
  close(fd);

  if (flagcd)				/* end multipart/mixed */
    hdr_boundary(1);

  strnum[fmt_ulong(strnum,when)] = 0;
  cookie(hash,key.s,key.len,strnum,addr.s,flagw ? "P" : "W");
  if (!stralloc_copy(&line,&outlocal)) die_nomem();
  if (!stralloc_cats(&line,flagw ? "-return-probe-" : "-return-warn-"))
	die_nomem();
  if (!stralloc_cats(&line,strnum)) die_nomem();
  if (!stralloc_cats(&line,".")) die_nomem();
  if (!stralloc_catb(&line,hash,COOKIE)) die_nomem();
  if (!stralloc_cats(&line,"-")) die_nomem();
  i = str_chr(addr.s,'@');
  if (!stralloc_catb(&line,addr.s,i)) die_nomem();
  if (addr.s[i]) {
    if (!stralloc_cats(&line,"=")) die_nomem();
    if (!stralloc_cats(&line,addr.s + i + 1)) die_nomem();
  }
  if (!stralloc_cats(&line,"@")) die_nomem();
  if (!stralloc_cat(&line,&outhost)) die_nomem();
  if (!stralloc_0(&line)) die_nomem();
  qmail_from(&qq,line.s);

  qmail_to(&qq,addr.s);
  if (*(err = qmail_close(&qq)) != '\0')
    strerr_die4x(111,FATAL,MSG(ERR_TMP_QMAIL_QUEUE),": ", err + 1);

  strnum[fmt_ulong(strnum,qmail_qp(&qq))] = 0;
  strerr_warn2("ezmlm-warn: info: qp ",strnum,0);

  if (!flagw) {
    if (unlink(fnhash.s) == -1)
      if (errno != error_noent)
        strerr_die2sys(111,FATAL,MSG1(ERR_DELETE,fnhash.s));
  }
  if (unlink(fn.s) == -1)
    strerr_die2sys(111,FATAL,MSG1(ERR_DELETE,fn.s));
}

static void dodir(int flagdig)
{
  DIR *bouncedir, *bsdir, *hdir;
  direntry *d, *ds;
  unsigned long bouncedate;
  unsigned long ld;
  unsigned long ddir,dfile;
  int fdlock,fd;
  char ch;

  workdir = flagdig ? "digest" : ".";

  if (!stralloc_copys(&fnlastd,workdir)) die_nomem();
  if (!stralloc_cats(&fnlastd,"/bounce/lastd")) die_nomem();
  if (!stralloc_0(&fnlastd)) die_nomem();
  if (slurp(fnlastd.s,&lastd,16) == -1)		/* last time d was scanned */
      strerr_die2sys(111,FATAL,MSG1(ERR_READ,fnlastd.s));
  if (!stralloc_0(&lastd)) die_nomem();
  (void) scan_ulong(lastd.s,&ld);
  if (!lockout)
    lockout = bouncetimeout / 50;		/* 5.6 h for default timeout */
  if (ld + lockout > when && ld < when)
    return;		/* exit silently. Second check is to prevent lockup */
			/* if lastd gets corrupted */

  if (!stralloc_copy(&fnlasth,&fnlastd)) die_nomem();
  fnlasth.s[fnlasth.len - 2] = 'h';		/* bad, but feels good ... */

  if (flagdig)
    if (!stralloc_cats(&outlocal,"-digest")) die_nomem();

  ddir = when / 10000;
  dfile = when - 10000 * ddir;

  if (!stralloc_copys(&line,workdir)) die_nomem();
  if (!stralloc_cats(&line,"/lockbounce")) die_nomem();
  if (!stralloc_0(&line)) die_nomem();
  fdlock = lockfile(line.s);

  if (!stralloc_copys(&line,workdir)) die_nomem();
  if (!stralloc_cats(&line,"/bounce/d")) die_nomem();
  if (!stralloc_0(&line)) die_nomem();
  bouncedir = opendir(line.s);
  if (!bouncedir) {
    if (errno != error_noent)
      strerr_die2sys(111,FATAL,MSG1(ERR_OPEN,line.s));
    else
      return;		/* no bouncedir - no bounces! */
  }

  initsub(0);

  while ((d = readdir(bouncedir))) {		/* dxxx/ */
    if (str_equal(d->d_name,".")) continue;
    if (str_equal(d->d_name,"..")) continue;

    scan_ulong(d->d_name,&bouncedate);
	/* since we do entire dir, we do files that are not old enough. */
	/* to not do this and accept a delay of 10000s (2.8h) of the oldest */
	/* bounce we add to bouncedate. We don't if bouncetimeout=0 so that */
	/* that setting still processes _all_ bounces. */
    if (bouncetimeout) ++bouncedate;
    if (when >= bouncedate * 10000 + bouncetimeout) {
      if (!stralloc_copys(&bdname,workdir)) die_nomem();
      if (!stralloc_cats(&bdname,"/bounce/d/")) die_nomem();
      if (!stralloc_cats(&bdname,d->d_name)) die_nomem();
      if (!stralloc_0(&bdname)) die_nomem();
      bsdir = opendir(bdname.s);
      if (!bsdir) {
	if (errno != error_notdir)
	  strerr_die2sys(111,FATAL,MSG1(ERR_OPEN,bdname.s));
	else {				/* leftover nnnnn_dmmmmm file */
	  if (unlink(bdname.s) == -1)
	    strerr_die2sys(111,FATAL,MSG1(ERR_DELETE,bdname.s));
	  continue;
	}
      }
      while ((ds = readdir(bsdir))) {			/* dxxxx/yyyy */
	if (str_equal(ds->d_name,".")) continue;
	if (str_equal(ds->d_name,"..")) continue;
	if (!stralloc_copy(&fn,&bdname)) die_nomem();	/* '\0' at end */
	  fn.s[fn.len - 1] = '/';
	if (!stralloc_cats(&fn,ds->d_name)) die_nomem();
	if (!stralloc_0(&fn)) die_nomem();
	if ((ds->d_name[0] == 'd') || (ds->d_name[0] == 'w'))
	  doit(flagdig, ds->d_name[0] == 'w');
        else				/* other stuff is junk */
	  if (unlink(fn.s) == -1)
	    strerr_die2sys(111,FATAL,MSG1(ERR_DELETE,fn.s));
      }
      closedir(bsdir);
      if (rmdir(bdname.s) == -1)	/* the directory itself */
      if (errno != error_noent)
	   strerr_die2sys(111,FATAL,MSG1(ERR_DELETE,bdname.s));
    }
  }
  closedir(bouncedir);

  if (!stralloc_copy(&line,&fnlastd)) die_nomem();
  line.s[line.len - 2] = 'D';
  fd = open_trunc(line.s);			/* write lastd. Do safe */
						/* since we read before lock*/
  if (fd == -1) strerr_die2sys(111,FATAL,MSG1(ERR_OPEN,line.s));
  substdio_fdbuf(&ssout,write,fd,outbuf,sizeof(outbuf));
  if (substdio_put(&ssout,strnum,fmt_ulong(strnum,when)) == -1)
    strerr_die2sys(111,FATAL,MSG1(ERR_WRITE,line.s));
  if (substdio_put(&ssout,"\n",1) == -1)	/* prettier */
    strerr_die2sys(111,FATAL,MSG1(ERR_WRITE,line.s));
  if (substdio_flush(&ssout) == -1)
    strerr_die2sys(111,FATAL,MSG1(ERR_FLUSH,line.s));
  if (fsync(fd) == -1)
    strerr_die2sys(111,FATAL,MSG1(ERR_SYNC,line.s));
  if (close(fd) == -1)
    strerr_die2sys(111,FATAL,MSG1(ERR_CLOSE,line.s));

  wrap_rename(line.s,fnlastd.s);

				/* no need to do h dir cleaning more than */
				/* once per 1-2 days (17-30 days for all) */
  if (stat(fnlasth.s,&st) == -1) {
    if (errno != error_noent)
      strerr_die2sys(111,FATAL,MSG1(ERR_STAT,fnlasth.s));
  } else if (when < (unsigned long)st.st_mtime + 100000
	     && when > (unsigned long)st.st_mtime)
    return;			/* 2nd comp to guard against corruption */

  if (slurp(fnlasth.s,&lasth,16) == -1)		/* last h cleaned */
      strerr_die2sys(111,FATAL,MSG1(ERR_READ,fnlasth.s));
  if (!stralloc_0(&lasth)) die_nomem();
  ch = lasth.s[0];				 /* clean h */
  if (ch >= 'a' && ch <= 'o')
    ++ch;
  else
    ch = 'a';
  lasth.s[0] = ch;
  if (!stralloc_copys(&line,workdir)) die_nomem();
  if (!stralloc_cats(&line,"/bounce/h/")) die_nomem();
  if (!stralloc_catb(&line,lasth.s,1)) die_nomem();
  if (!stralloc_0(&line)) die_nomem();
  hdir = opendir(line.s);		/* clean ./h/xxxxxx */

  if (!hdir) {
    if (errno != error_noent)
    strerr_die2sys(111,FATAL,MSG1(ERR_OPEN,line.s));
  } else {

    while ((d = readdir(hdir))) {
      if (str_equal(d->d_name,".")) continue;
      if (str_equal(d->d_name,"..")) continue;
      if (!stralloc_copys(&fn,line.s)) die_nomem();
      if (!stralloc_append(&fn,"/")) die_nomem();
      if (!stralloc_cats(&fn,d->d_name)) die_nomem();
      if (!stralloc_0(&fn)) die_nomem();
      if (stat(fn.s,&st) == -1) {
	if (errno == error_noent) continue;
	strerr_die2sys(111,FATAL,MSG1(ERR_STAT,fn.s));
      }
      if (when > st.st_mtime + 3 * bouncetimeout)
	if (unlink(fn.s) == -1)
          strerr_die2sys(111,FATAL,MSG1(ERR_DELETE,fn.s));
    }
    closedir(hdir);
  }

  fd = open_trunc(fnlasth.s);			/* write lasth */
  if (fd == -1) strerr_die2sys(111,FATAL,MSG1(ERR_OPEN,fnlasth.s));
  substdio_fdbuf(&ssout,write,fd,outbuf,sizeof(outbuf));
  if (substdio_put(&ssout,lasth.s,1) == -1)
    strerr_die2sys(111,FATAL,MSG1(ERR_OPEN,fnlasth.s));
  if (substdio_put(&ssout,"\n",1) == -1)	/* prettier */
    strerr_die2sys(111,FATAL,MSG1(ERR_OPEN,fnlasth.s));
  if (substdio_flush(&ssout) == -1)
    strerr_die2sys(111,FATAL,MSG1(ERR_OPEN,fnlasth.s));
  (void) close(fd);		/* no big loss. No reason to flush/sync */
				/* See check of ld above to guard against */
				/* it being corrupted and > when */

  closesub();
}

int main(int argc,char **argv)
{
  (void) umask(022);
  sig_pipeignore();
  when = (unsigned long) now();
  getconfopt(argc,argv,options,1,0);
  bouncetimeout *= 3600L * 24L;
  if (nowarn)
    _exit(0);
  if (flagdig < 0) {
    dodir(0);
    if (getconf_isset("digest"))
      dodir(1);
  }
  else
    dodir(flagdig);
  _exit(0);
}
