/*$Id$*/
#include <sys/types.h>
#include <sys/stat.h>
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
#include "date822fmt.h"
#include "fmt.h"
#include "cookie.h"
#include "qmail.h"
#include "errtxt.h"
#include "mime.h"
#include "idx.h"
#include "subscribe.h"

#define FATAL "ezmlm-warn: fatal: "
void die_usage()
{
  strerr_die1x(100,"ezmlm-warn: usage: ezmlm-warn -dD -l secs -t days dir");
}

void die_nomem() { strerr_die2x(111,FATAL,ERR_NOMEM); }

stralloc key = {0};
stralloc outhost = {0};
stralloc outlocal = {0};
stralloc mailinglist = {0};
stralloc digdir = {0};
stralloc charset = {0};
char boundary[COOKIE];

substdio ssout;
char outbuf[16];

unsigned long when;
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
void *psql = (void *) 0;

void die_read() { strerr_die4sys(111,FATAL,ERR_READ,fn.s,": "); }

void makedir(s)
char *s;
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
int qqwrite(fd,buf,len) int fd; char *buf; unsigned int len;
{
  qmail_put(&qq,buf,len);
  return len;
}
char qqbuf[1];
substdio ssqq = SUBSTDIO_FDBUF(qqwrite,-1,qqbuf,sizeof(qqbuf));
struct datetime dt;
char date[DATE822FMT];

void code_qput(s,n)
char *s;
unsigned int n;
{
    if (!flagcd)
      qmail_put(&qq,s,n);
    else {
      if (flagcd == 'B')
        encodeB(s,n,&qline,0,FATAL);
      else
        encodeQ(s,n,&qline,FATAL);
      qmail_put(&qq,qline.s,qline.len);
    }
}

void doit(flagw)
int flagw;
{
  unsigned int i;
  int fd;
  int match;
  int fdhash;
  char *err;
  datetime_sec msgwhen;

  fd = open_read(fn.s);
  if (fd == -1) die_read();
  substdio_fdbuf(&ssin,read,fd,inbuf,sizeof(inbuf));

  if (getln(&ssin,&addr,&match,'\0') == -1) die_read();
  if (!match) { close(fd); return; }
  if (!issub(workdir,addr.s,(char *) 0,FATAL)) { close(fd);
			 /*XXX*/unlink(fn.s); return; }
  cookie(hash,"",0,"",addr.s,"");
  if (!stralloc_copys(&fnhash,workdir)) die_nomem();
  if (!stralloc_cats(&fnhash,"/bounce/h/")) die_nomem();
  if (!stralloc_catb(&fnhash,hash,1)) die_nomem();
  if (!stralloc_cats(&fnhash,"/h")) die_nomem();
  if (!stralloc_catb(&fnhash,hash+1,COOKIE-1)) die_nomem();
  if (!stralloc_0(&fnhash)) die_nomem();

  if (qmail_open(&qq, (stralloc *) 0) == -1)
    strerr_die2sys(111,FATAL,ERR_QMAIL_QUEUE);

  msgwhen = now();
  qmail_puts(&qq,"Mailing-List: ");
  qmail_put(&qq,mailinglist.s,mailinglist.len);
  if (getconf_line(&line,"listid",0,FATAL,dir)) {
    qmail_puts(&qq,"\nList-ID: ");
    qmail_put(&qq,line.s,line.len);
  }
  qmail_puts(&qq,"\nDate: ");
  datetime_tai(&dt,msgwhen);
  qmail_put(&qq,date,date822fmt(date,&dt));
  if (!stralloc_copys(&line,"Message-ID: <")) die_nomem();
  if (!stralloc_catb(&line,strnum,fmt_ulong(strnum,(unsigned long) msgwhen)))
		die_nomem();
  if (!stralloc_cats(&line,".")) die_nomem();
  if (!stralloc_catb(&line,strnum,fmt_ulong(strnum,(unsigned long) getpid())))
		die_nomem();
  if (!stralloc_cats(&line,".ezmlm-warn@")) die_nomem();
  if (!stralloc_catb(&line,outhost.s,outhost.len)) die_nomem();
  qmail_put(&qq,line.s,line.len);
  if (flagcd) {
    if (!stralloc_0(&line)) die_nomem();
    cookie(boundary,"",0,"",line.s,"");	/* universal MIME boundary */
  }
  qmail_puts(&qq,">\nFrom: ");
  if (!quote(&quoted,&outlocal)) die_nomem();
  qmail_put(&qq,quoted.s,quoted.len);
  qmail_puts(&qq,"-help@");
  qmail_put(&qq,outhost.s,outhost.len);
  qmail_puts(&qq,"\nTo: ");
  if (!quote2(&quoted,addr.s)) die_nomem();
  qmail_put(&qq,quoted.s,quoted.len);
  if (flagcd) {			/* to accomodate transfer-encoding */
    qmail_puts(&qq,"\nMIME-Version: 1.0\n");
    qmail_puts(&qq,"Content-Type: multipart/mixed; boundary=");
    qmail_put(&qq,boundary,COOKIE);
  } else {
    qmail_puts(&qq,"\nContent-type: text/plain; charset=");
    qmail_puts(&qq,charset.s);
  }
  qmail_puts(&qq,flagw ? "\nSubject: ezmlm probe\n" : "\nSubject: ezmlm warning\n");

  if (flagcd) {			/* first part for QP/base64 multipart msg */
    qmail_puts(&qq,"\n\n--");
    qmail_put(&qq,boundary,COOKIE);
    qmail_puts(&qq,"\nContent-Type: text/plain; charset=");
    qmail_puts(&qq,charset.s);
    qmail_puts(&qq,"\nContent-Transfer-Encoding: ");
    if (flagcd == 'Q')
      qmail_puts(&qq,"Quoted-printable\n\n");
    else
      qmail_puts(&qq,"base64\n\n");
  } else
    qmail_puts(&qq,"\n");

  copy(&qq,"text/top",flagcd,FATAL);
  copy(&qq,flagw ? "text/bounce-probe" : "text/bounce-warn",flagcd,FATAL);

  if (!flagw) {
    if (flagdig)
      copy(&qq,"text/dig-bounce-num",flagcd,FATAL);
    else
      copy(&qq,"text/bounce-num",flagcd,FATAL);
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

  copy(&qq,"text/bounce-bottom",flagcd,FATAL);
  if (flagcd) {
    if (flagcd == 'B') {
      encodeB("",0,&line,2,FATAL);
      qmail_put(&qq,line.s,line.len);	/* flush */
    }
    qmail_puts(&qq,"\n\n--");
    qmail_put(&qq,boundary,COOKIE);
    qmail_puts(&qq,"\nContent-Type: message/rfc822\n\n");
  }
  if (substdio_copy(&ssqq,&ssin) < 0) die_read();
  close(fd);

  if (flagcd) {				/* end multipart/mixed */
    qmail_puts(&qq,"\n--");
    qmail_put(&qq,boundary,COOKIE);
    qmail_puts(&qq,"--\n");
  }

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

void main(argc,argv)
int argc;
char **argv;
{
  DIR *bouncedir, *bsdir, *hdir;
  direntry *d, *ds;
  unsigned long bouncedate;
  unsigned long bouncetimeout = BOUNCE_TIMEOUT;
  unsigned long lockout = 0L;
  unsigned long ld;
  unsigned long ddir,dfile;
  int fdlock,fd;
  char *err;
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
      case 'V': strerr_die2x(0,
		"ezmlm-warn version: ezmlm-0.53+",EZIDX_VERSION);
      default:
	die_usage();
    }
  dir = argv[optind];
  if (!dir) die_usage();
  if (chdir(dir) == -1)
    strerr_die4sys(111,FATAL,ERR_SWITCH,dir,": ");
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

  switch(slurp("key",&key,32)) {
    case -1:
      strerr_die4sys(111,FATAL,ERR_READ,dir,"/key: ");
    case 0:
      strerr_die4x(100,FATAL,dir,"/key",ERR_NOEXIST);
  }
  getconf_line(&outhost,"outhost",1,FATAL,dir);
  getconf_line(&outlocal,"outlocal",1,FATAL,dir);
  if (flagdig)
    if (!stralloc_cats(&outlocal,"-digest")) die_nomem();
  getconf_line(&mailinglist,"mailinglist",1,FATAL,dir);
  if (getconf_line(&charset,"charset",0,FATAL,dir)) {
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
  fdlock = open_append(line.s);
  if (fdlock == -1)
    strerr_die4sys(111,FATAL,ERR_OPEN,line.s,": ");
  if (lock_ex(fdlock) == -1)
    strerr_die4sys(111,FATAL,ERR_OBTAIN,line.s,": ");

  if (!stralloc_copys(&line,workdir)) die_nomem();
  if (!stralloc_cats(&line,"/bounce/d")) die_nomem();
  if (!stralloc_0(&line)) die_nomem();
  bouncedir = opendir(line.s);
  if (!bouncedir)
    if (errno != error_noent)
      strerr_die4sys(111,FATAL,ERR_OPEN,line.s,": ");
    else
      _exit(0);		/* no bouncedir - no bounces! */

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

  closesql();
  _exit(0);
}
