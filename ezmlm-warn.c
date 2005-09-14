/*$Id$*/

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
#include "sgetopt.h"
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
#include "errtxt.h"
#include "quote.h"
#include "open.h"
#include "scan.h"
#include "lock.h"
#include "copy.h"
#include "mime.h"
#include "auto_version.h"
#include "hdr.h"
#include "die.h"
#include "idx.h"
#include "config.h"
#include "subscribe.h"

const char FATAL[] = "ezmlm-warn: fatal: ";
const char USAGE[] =
"ezmlm-warn: usage: ezmlm-warn -dD -l secs -t days dir";

stralloc digdir = {0};
stralloc charset = {0};
char boundary[COOKIE];

substdio ssout;
char outbuf[16];

long when;
char *dir;
char *workdir;
int flagdig = 0;
char flagcd = '\0';		/* default: don't use transfer encoding */
stralloc fn = {0};
stralloc bdname = {0};
stralloc fnlasth = {0};
stralloc fnlastd = {0};
stralloc lasth = {0};
stralloc lastd = {0};
struct stat st;

static void die_read(void) { strerr_die4sys(111,FATAL,ERR_READ,fn.s,": "); }

void makedir(const char *s)
{
  if (mkdir(s,0755) == -1)
    if (errno != error_exist)
      strerr_die4x(111,FATAL,ERR_CREATE,s,": ");
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

void code_qput(const char *s,unsigned int n)
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

void doit(int flagw)
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
  if (!issub(workdir,0,addr.s)) { close(fd); /*XXX*/unlink(fn.s); return; }
  cookie(hash,"",0,"",addr.s,"");
  if (!stralloc_copys(&fnhash,workdir)) die_nomem();
  if (!stralloc_cats(&fnhash,"/bounce/h/")) die_nomem();
  if (!stralloc_catb(&fnhash,hash,1)) die_nomem();
  if (!stralloc_cats(&fnhash,"/h")) die_nomem();
  if (!stralloc_catb(&fnhash,hash+1,COOKIE-1)) die_nomem();
  if (!stralloc_0(&fnhash)) die_nomem();

  if (qmail_open(&qq, (stralloc *) 0) == -1)
    strerr_die2sys(111,FATAL,ERR_QMAIL_QUEUE);

  hdr_add2("Mailing-List: ",mailinglist.s,mailinglist.len);
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
  hdr_listsubject1(flagw ? "probe from " : "warning from ");

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
          strerr_die4sys(111,FATAL,ERR_OPEN,fnhash.s,": ");
      } else {
        substdio_fdbuf(&sstext,read,fdhash,textbuf,sizeof(textbuf));
        for(;;) {
          if (getln(&sstext,&line,&match,'\n') == -1)
            strerr_die4sys(111,FATAL,ERR_READ,fnhash.s,": ");
          if (!match) break;
          code_qput(line.s,line.len);
        }
      }
      close(fdhash);
    } else {
      if (!stralloc_copys(&line,"")) die_nomem();	/* slurp adds! */
      if (slurp(fnhash.s,&line,256) < 0)
        strerr_die4sys(111,FATAL,ERR_OPEN,fnhash.s,": ");
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
  if (qmail_copy(&qq,&ssin) < 0) die_read();
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
    strerr_die3x(111,FATAL,ERR_TMP_QMAIL_QUEUE, err + 1);

  strnum[fmt_ulong(strnum,qmail_qp(&qq))] = 0;
  strerr_warn2("ezmlm-warn: info: qp ",strnum,0);

  if (!flagw) {
    if (unlink(fnhash.s) == -1)
      if (errno != error_noent)
        strerr_die4sys(111,FATAL,ERR_DELETE,fnhash.s,": ");
  }
  if (unlink(fn.s) == -1)
    strerr_die4sys(111,FATAL,ERR_DELETE,fn.s,": ");
}

void main(int argc,char **argv)
{
  DIR *bouncedir, *bsdir, *hdir;
  direntry *d, *ds;
  long bouncedate;
  long bouncetimeout = BOUNCE_TIMEOUT;
  long lockout = 0L;
  long ld;
  unsigned long ddir,dfile;
  int fdlock,fd;
  int opt;
  char ch;

  (void) umask(022);
  sig_pipeignore();
  when = (unsigned long) now();
  while ((opt = getopt(argc,argv,"dDl:t:vV")) != opteof)
    switch(opt) {
      case 'd': flagdig = 1; break;
      case 'D': flagdig = 0; break;
      case 'l':
                if (optarg) {	/* lockout in seconds */
                  (void) scan_ulong(optarg,&lockout);
                }
                break;
      case 't':
                if (optarg) {	/* bouncetimeout in days */
                  (void) scan_ulong(optarg,&bouncetimeout);
                  bouncetimeout *= 3600L * 24L;
                }
                break;
      case 'v':
      case 'V': strerr_die2x(0, "ezmlm-warn version: ",auto_version);
      default:
	die_usage();
    }
  startup(dir = argv[optind]);
  load_config(dir);
  if (flagdig) {
    if (!stralloc_copys(&digdir,dir)) die_nomem();
    if (!stralloc_cats(&digdir,"/digest")) die_nomem();
    if (!stralloc_0(&digdir)) die_nomem();
    workdir = digdir.s;
  } else
    workdir = dir;

  if (!stralloc_copys(&fnlastd,workdir)) die_nomem();
  if (!stralloc_cats(&fnlastd,"/bounce/lastd")) die_nomem();
  if (!stralloc_0(&fnlastd)) die_nomem();
  if (slurp(fnlastd.s,&lastd,16) == -1)		/* last time d was scanned */
      strerr_die4sys(111,FATAL,ERR_READ,fnlastd.s,": ");
  if (!stralloc_0(&lastd)) die_nomem();
  (void) scan_ulong(lastd.s,&ld);
  if (!lockout)
    lockout = bouncetimeout / 50;		/* 5.6 h for default timeout */
  if (ld + lockout > when && ld < when)
    _exit(0);		/* exit silently. Second check is to prevent lockup */
			/* if lastd gets corrupted */

  if (!stralloc_copy(&fnlasth,&fnlastd)) die_nomem();
  fnlasth.s[fnlasth.len - 2] = 'h';		/* bad, but feels good ... */

  if (flagdig)
    if (!stralloc_cats(&outlocal,"-digest")) die_nomem();
  if (getconf_line(&charset,"charset",0,dir)) {
    if (charset.len >= 2 && charset.s[charset.len - 2] == ':') {
      if (charset.s[charset.len - 1] == 'B' ||
		charset.s[charset.len - 1] == 'Q') {
        flagcd = charset.s[charset.len - 1];
        charset.s[charset.len - 2] = '\0';
      }
    }
  } else
    if (!stralloc_copys(&charset,TXT_DEF_CHARSET)) die_nomem();
  if (!stralloc_0(&charset)) die_nomem();

  set_cpoutlocal(&outlocal);	/* for copy */
  set_cpouthost(&outhost);	/* for copy */
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
      strerr_die4sys(111,FATAL,ERR_OPEN,line.s,": ");
    else
      _exit(0);		/* no bouncedir - no bounces! */
  }

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
	  strerr_die4sys(111,FATAL,ERR_OPEN,bdname.s,":y ");
	else {				/* leftover nnnnn_dmmmmm file */
	  if (unlink(bdname.s) == -1)
	    strerr_die4sys(111,FATAL,ERR_DELETE,bdname.s,": ");
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
	  doit(ds->d_name[0] == 'w');
        else				/* other stuff is junk */
	  if (unlink(fn.s) == -1)
	    strerr_die4sys(111,FATAL,ERR_DELETE,fn.s,": ");
      }
      closedir(bsdir);
      if (rmdir(bdname.s) == -1)	/* the directory itself */
      if (errno != error_noent)
	   strerr_die4sys(111,FATAL,ERR_DELETE,bdname.s,": ");
    }
  }
  closedir(bouncedir);

  if (!stralloc_copy(&line,&fnlastd)) die_nomem();
  line.s[line.len - 2] = 'D';
  fd = open_trunc(line.s);			/* write lastd. Do safe */
						/* since we read before lock*/
  if (fd == -1) strerr_die4sys(111,FATAL,ERR_OPEN,line.s,": ");
  substdio_fdbuf(&ssout,write,fd,outbuf,sizeof(outbuf));
  if (substdio_put(&ssout,strnum,fmt_ulong(strnum,when)) == -1)
    strerr_die4sys(111,FATAL,ERR_WRITE,line.s,": ");
  if (substdio_put(&ssout,"\n",1) == -1)	/* prettier */
    strerr_die4sys(111,FATAL,ERR_WRITE,line.s,": ");
  if (substdio_flush(&ssout) == -1)
    strerr_die4sys(111,FATAL,ERR_FLUSH,line.s,": ");
  if (fsync(fd) == -1)
    strerr_die4sys(111,FATAL,ERR_SYNC,line.s,": ");
  if (close(fd) == -1)
    strerr_die4sys(111,FATAL,ERR_CLOSE,line.s,": ");

  if (rename(line.s,fnlastd.s) == -1)
    strerr_die4sys(111,FATAL,ERR_MOVE,fnlastd.s,": ");

				/* no need to do h dir cleaning more than */
				/* once per 1-2 days (17-30 days for all) */
  if (stat(fnlasth.s,&st) == -1) {
    if (errno != error_noent)
      strerr_die4sys(111,FATAL,ERR_STAT,fnlasth.s,": ");
  } else if (when < st.st_mtime + 100000 && when > st.st_mtime)
    _exit(0);			/* 2nd comp to guard against corruption */

  if (slurp(fnlasth.s,&lasth,16) == -1)		/* last h cleaned */
      strerr_die4sys(111,FATAL,ERR_READ,fnlasth.s,": ");
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
    strerr_die4sys(111,FATAL,ERR_OPEN,line.s,": ");
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
	strerr_die4sys(111,FATAL,ERR_STAT,fn.s,": ");
      }
      if (when > st.st_mtime + 3 * bouncetimeout)
	if (unlink(fn.s) == -1)
          strerr_die4sys(111,FATAL,ERR_DELETE,fn.s,": ");
    }
    closedir(hdir);
  }

  fd = open_trunc(fnlasth.s);			/* write lasth */
  if (fd == -1) strerr_die4sys(111,FATAL,ERR_OPEN,fnlasth.s,": ");
  substdio_fdbuf(&ssout,write,fd,outbuf,sizeof(outbuf));
  if (substdio_put(&ssout,lasth.s,1) == -1)
    strerr_die4sys(111,FATAL,ERR_OPEN,fnlasth.s,": ");
  if (substdio_put(&ssout,"\n",1) == -1)	/* prettier */
    strerr_die4sys(111,FATAL,ERR_OPEN,fnlasth.s,": ");
  if (substdio_flush(&ssout) == -1)
    strerr_die4sys(111,FATAL,ERR_OPEN,fnlasth.s,": ");
  (void) close(fd);		/* no big loss. No reason to flush/sync */
				/* See check of ld above to guard against */
				/* it being corrupted and > when */

  closesub();
  _exit(0);
}
