/* Handles receipts and bounces from sublists at the main list */
/* Set up instead of ezmlm-return in DIR/bouncer of main list */

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <unistd.h>
#include "sys/direntry.h"
#include "stralloc.h"
#include "str.h"
#include "env.h"
#include "sender.h"
#include "slurp.h"
#include "strerr.h"
#include "byte.h"
#include "case.h"
#include "quote.h"
#include "getln.h"
#include "getconfopt.h"
#include "substdio.h"
#include "error.h"
#include "readwrite.h"
#include "fmt.h"
#include "now.h"
#include "seek.h"
#include "die.h"
#include "idx.h"
#include "config.h"
#include "messages.h"
#include "auto_version.h"

const char FATAL[] = "ezmlm-receipt: fatal: ";
const char INFO[] = "ezmlm-receipt: info: ";
const char USAGE[] =
"ezmlm-receipt: usage: ezmlm-receipt [-dD] dir";

static int flagdig = 1;

static struct option options[] = {
  OPT_FLAG(flagdig,'d',2,0),
  OPT_FLAG(flagdig,'d',0,0),
  OPT_END
};

static void die_trash(void)
{
  strerr_die2x(0,INFO,"trash address");
}

static stralloc fndate = {0};
static stralloc fndatenew = {0};
static stralloc fndir = {0};
static stralloc line = {0};
static stralloc quoted = {0};

static void die_datenew(void)
{ strerr_die2sys(111,FATAL,MSG1(ERR_WRITE,fndatenew.s)); }
static void die_msgin(void)
{ strerr_die2sys(111,FATAL,MSG(ERR_READ_INPUT)); }

static char outbuf[256];	/* small - rarely used */
static substdio ssout;

static unsigned long addrno = 0L;

static const char *sender;


static void doit(const char *workdir,const char *addr,unsigned long msgnum,unsigned long when,const stralloc *bounce)
/* Just stores address\0nsgnum\0 followed by bounce. File name is          */
/* dttt.ppp[.n], where 'ttt' is a time stamp, 'ppp' the pid, and 'n' the   */
/* number when there are more than 1 addresses in a pre-VERP bounce. In    */
/* this case, the first one is just dttt.ppp, the decond dttt.ppp.2, etc.  */
/* For a main list, bounces come from sublists. They are rare and serious. */
{
  int fd;
  unsigned int pos;
  DIR *bouncedir;
  direntry *d;
  unsigned int no;
  char strnum[FMT_ULONG];

  stralloc_copys(&fndir,workdir);
  stralloc_cats(&fndir,"/bounce");
  stralloc_0(&fndir);
  bouncedir = opendir(fndir.s);
  if (!bouncedir)
    if (errno != error_noent)
      strerr_die2sys(111,FATAL,MSG1(ERR_OPEN,line.s));
    else
      strerr_die3x(111,FATAL,fndir.s,MSG(ERR_NOEXIST));

  no = MAX_MAIN_BOUNCES;	/* no more than this many allowed */
  while (no && (d = readdir(bouncedir))) {
    if (str_equal(d->d_name,".")) continue;
    if (str_equal(d->d_name,"..")) continue;
    --no;
  }
  closedir(bouncedir);
  if (!no)			/* max no of bounces exceeded */
    strerr_die2x(0,INFO,MSG(ERR_MAX_BOUNCE));
				/* save bounce */
  stralloc_copys(&fndate,workdir);
  stralloc_cats(&fndate,"/bounce/d");
  pos = fndate.len - 1;
  stralloc_catb(&fndate,strnum,fmt_ulong(strnum,when));
  stralloc_cats(&fndate,".");
  stralloc_catb(&fndate,strnum,fmt_ulong(strnum,(unsigned long) getpid()));
  if (addrno) {	/* so that pre-VERP bounces make a d... file per address */
		/* for the first one we use the std-style fname */
    stralloc_cats(&fndate,".");
    stralloc_catb(&fndate,strnum,fmt_ulong(strnum,addrno));
  }
  addrno++;	/* get ready for next */
  stralloc_0(&fndate);
  stralloc_copy(&fndatenew,&fndate);
  fndatenew.s[pos] = 'D';

  fd = open_trunc(fndatenew.s);
  if (fd == -1) die_datenew();
  substdio_fdbuf(&ssout,write,fd,outbuf,sizeof(outbuf));
  if (substdio_puts(&ssout,addr) == -1) die_datenew();
  if (substdio_put(&ssout,"",1) == -1) die_datenew();
  if (substdio_put(&ssout,strnum,fmt_ulong(strnum,msgnum)) == -1)
	die_datenew();
  if (substdio_put(&ssout,"",1) == -1) die_datenew();

  if (substdio_puts(&ssout,"Return-Path: <") == -1) die_datenew();
  if (!quote2(&quoted,sender)) die_nomem();
  if (substdio_put(&ssout,quoted.s,quoted.len) == -1) die_datenew();
  if (substdio_puts(&ssout,">\n") == -1) die_datenew();
  if (substdio_put(&ssout,bounce->s,bounce->len) == -1) die_datenew();
  if (substdio_flush(&ssout) == -1) die_datenew();
  if (fsync(fd) == -1) die_datenew();
  if (close(fd) == -1) die_datenew(); /* NFS stupidity */
  wrap_rename(fndatenew.s,fndate.s);
}

int main(int argc,char **argv)
{
  stralloc intro = {0};
  stralloc bounce = {0};
  stralloc header = {0};
  stralloc failure = {0};
  stralloc paragraph = {0};
  stralloc ddir = {0};
  stralloc tagline = {0};
  stralloc listaddr = {0};

  char inbuf[1024];
  substdio ssin;

  unsigned long when;

  const char *dir;
  const char *workdir;
  stralloc listno = {0};

  char *local;
  char *host;
  char *action;
  int flaghaveintro;
  int flaghaveheader;
  int match;
  unsigned long msgnum;
  unsigned int i;
  unsigned int len;
  char *cp;

  umask(022);
  sig_pipeignore();

  when = (unsigned long) now();

  getconfopt(argc,argv,options,1,&dir);

  sender = get_sender();
  action = env_get("DEFAULT");
  local = env_get("LOCAL");
  if (!action) strerr_die2x(100,FATAL,MSG(ERR_NODEFAULT));

  workdir = dir;
  /* now see if -digest-return- */
  if (flagdig == 1) {
    flagdig = 0;
    if (str_len(local) >= str_len(action) + 14)
      if (str_start(local + str_len(local) - 14 - str_len(action),"digest-"))
	flagdig = 2;
  }
  if (flagdig) {
    stralloc_copys(&ddir,dir);
    stralloc_cats(&ddir,"/digest");
    stralloc_0(&ddir);
    workdir = ddir.s;
    stralloc_cats(&outlocal,"-digest");
  }
  if (!*action) die_trash();

  substdio_fdbuf(&ssin,read,0,inbuf,sizeof(inbuf));

  if (!case_diffs(action,"receipt")) {
    host = sender + str_rchr(sender,'@');
    if (*host)
      *(host++) = '\0';
    cp = sender;
				/* check recipient in case it's a bounce*/
    while (*(cp++)) {		/* decode sender */
      cp += str_chr(cp,'-');
      if (case_starts(cp,"-return-")) {
        if (!scan_ulong(cp + 8,&msgnum))
          strerr_die2x(100,FATAL,"bad VERP format for receipt");
         *cp = '\0';
        stralloc_copys(&listaddr,sender);
        stralloc_append(&listaddr,'@');
        stralloc_cats(&listaddr,host);
        stralloc_0(&listaddr);
        break;
      }
    }
    for(;;) {						/* Get X-tag from hdr*/
      if (getln(&ssin,&line,&match,'\n') == -1) die_msgin();
      if (!match)
        break;

      if (line.len == 1) break;
      if (case_startb(line.s,line.len,TXT_TAG)) {
	len = str_len(TXT_TAG);
        stralloc_catb(&tagline,line.s +len,line.len - len -1);
		/* NOTE: tagline is dirty! We quote it for sql and rely on */
		/* std log clean for maillog */
        break;
      }
    }
		/* feedback ok even if not sub. Will be filtered by subreceipt*/
		/* For instance, main list feedback is ok, but !issub. */
    subreceipt(workdir,msgnum,&tagline,listaddr.s,2,INFO,FATAL);
    closesub();
    _exit(0);
  }
				/* not receipt - maybe bounce */
				/* no need to lock. dttt.pid can be assumed */
				/* to be unique and if not would be over- */
				/* written even with lock */
  action += scan_ulong(action,&msgnum);
  if (*action != '-') die_badaddr();
  ++action;
		/* scan bounce for tag. It'll be in the BODY! */
  for (;;) {
    if (getln(&ssin,&line,&match,'\n') == -1)
      strerr_die2sys(111,FATAL,MSG(ERR_READ_INPUT));
    if (!match) break;
    if (case_startb(line.s,line.len,TXT_TAG)) {
      len = str_len(TXT_TAG);
      stralloc_catb(&tagline,line.s +len,line.len - len -1);
		/* NOTE: tagline is dirty! We quote it for sql and rely on */
		/* std log clean for maillog */
      break;
    }
  }
  if (seek_begin(0) == -1)
    strerr_die2sys(111,FATAL,MSG(ERR_SEEK_INPUT));
  substdio_fdbuf(&ssin,read,0,inbuf,sizeof(inbuf));

  if (*action) {	/* normal bounce */

    if (slurpclose(0,&bounce,1024) == -1) die_msgin();
    i = str_rchr(action,'=');
    stralloc_copyb(&listaddr,action,i);
    if (action[i]) {
      stralloc_cats(&listaddr,"@");
      stralloc_cats(&listaddr,action + i + 1);
    }
    stralloc_0(&listaddr);
		/* don't check for sub, since issub() doesn't see sublists */
    switch (subreceipt(workdir,msgnum,&tagline,listaddr.s,-1,INFO,FATAL)) {
	case -1: strerr_die2x(0,INFO,MSG(ERR_COOKIE));
	case -2: strerr_die2x(0,INFO,MSG(ERR_NOT_ACTIVE));
        default: doit(workdir,listaddr.s,msgnum,when,&bounce);
    }
    closesub();
    _exit(0);
  }			/* pre-VERP bounce, in QSBMF format */

  flaghaveheader = 0;
  flaghaveintro = 0;

  for (;;) {
    stralloc_copys(&paragraph,"");
    for (;;) {
      if (getln(&ssin,&line,&match,'\n') == -1)
        strerr_die2sys(111,FATAL,MSG(ERR_READ_INPUT));
      if (!match) die_trash();
      stralloc_cat(&paragraph,&line);
      if (line.len <= 1) break;
    }

    if (!flaghaveheader) {
      stralloc_copy(&header,&paragraph);
      flaghaveheader = 1;
      continue;
    }

    if (!flaghaveintro) {
      if (paragraph.s[0] == '-' && paragraph.s[1] == '-')
        continue;		/* skip MIME boundary if it exists */
      if (paragraph.len < 15) die_trash();
      if (str_diffn(paragraph.s,"Hi. This is the",15)) die_trash();
      stralloc_copy(&intro,&paragraph);
      flaghaveintro = 1;
      continue;
    }

    if (paragraph.s[0] == '-')
      break;

    if (paragraph.s[0] == '<') {	/* find address */
      stralloc_copy(&failure,&paragraph);

      stralloc_copy(&bounce,&header);
      stralloc_cat(&bounce,&intro);
      stralloc_cat(&bounce,&failure);

      i = byte_chr(failure.s,failure.len,'\n');
      if (i < 3) die_trash();

      stralloc_copyb(&listaddr,failure.s + 1,i - 3);
      if (byte_chr(listaddr.s,listaddr.len,'\0') == listaddr.len) {
        stralloc_0(&listaddr);
        if (subreceipt(workdir,msgnum,&tagline,listaddr.s,-1,INFO,FATAL) == 0)
	  doit(workdir,listaddr.s,msgnum,when,&bounce);
      }
    }
  }
  closesub();
  _exit(0);
}

