/*$Id$*/
/* Handles receipts and bounces from sublists at the main list */
/* Set up instead of ezmlm-return in DIR/bouncer of main list */

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <unistd.h>
#include "direntry.h"
#include "stralloc.h"
#include "str.h"
#include "env.h"
#include "slurp.h"
#include "getconf.h"
#include "strerr.h"
#include "byte.h"
#include "case.h"
#include "quote.h"
#include "getln.h"
#include "substdio.h"
#include "error.h"
#include "readwrite.h"
#include "fmt.h"
#include "now.h"
#include "seek.h"
#include "idx.h"
#include "errtxt.h"

const char FATAL[] = "ezmlm-receipt: fatal: ";
#define INFO "ezmlm-receipt: info: "

void die_usage()
{
  strerr_die1x(100,"ezmlm-receipt: usage: ezmlm-receipt [-dD] dir");
}

void die_nomem() { strerr_die2x(111,FATAL,ERR_NOMEM); }

void die_badaddr()
{
  strerr_die2x(100,FATAL,ERR_BAD_ADDRESS);
}
void die_trash()
{
  strerr_die2x(0,INFO,"trash address");
}

stralloc line = {0};
stralloc quoted = {0};
stralloc intro = {0};
stralloc bounce = {0};
stralloc header = {0};
stralloc failure = {0};
stralloc paragraph = {0};
stralloc ddir = {0};
stralloc outhost = {0};
stralloc outlocal = {0};
stralloc tagline = {0};
stralloc listaddr = {0};
stralloc fndate = {0};
stralloc fndir = {0};
stralloc fndatenew = {0};

void die_datenew()
{ strerr_die4sys(111,FATAL,ERR_WRITE,fndatenew.s,": "); }
void die_msgin()
{ strerr_die2sys(111,FATAL,ERR_READ_INPUT); }

char strnum[FMT_ULONG];
char inbuf[1024];
substdio ssin;

char outbuf[256];	/* small - rarely used */
substdio ssout;

unsigned long when;
unsigned long addrno = 0L;

char *sender;
char *dir;
char *workdir;
void **psql = (void **) 0;
stralloc listno = {0};


void doit(addr,msgnum,when,bounce)
/* Just stores address\0nsgnum\0 followed by bounce. File name is          */
/* dttt.ppp[.n], where 'ttt' is a time stamp, 'ppp' the pid, and 'n' the   */
/* number when there are more than 1 addresses in a pre-VERP bounce. In    */
/* this case, the first one is just dttt.ppp, the decond dttt.ppp.2, etc.  */
/* For a main list, bounces come from sublists. They are rare and serious. */
char *addr;
unsigned long msgnum;
unsigned long when;
stralloc *bounce;
{
  int fd;
  unsigned int pos;
  DIR *bouncedir;
  direntry *d;
  unsigned int no;

  if (!stralloc_copys(&fndir,workdir)) die_nomem();
  if (!stralloc_cats(&fndir,"/bounce")) die_nomem();
  if (!stralloc_0(&fndir)) die_nomem();
  bouncedir = opendir(fndir.s);
  if (!bouncedir)
    if (errno != error_noent)
      strerr_die4sys(111,FATAL,ERR_OPEN,line.s,": ");
    else
      strerr_die3x(111,FATAL,fndir.s,ERR_NOEXIST);

  no = MAX_MAIN_BOUNCES;	/* no more than this many allowed */
  while (no && (d = readdir(bouncedir))) {
    if (str_equal(d->d_name,".")) continue;
    if (str_equal(d->d_name,"..")) continue;
    --no;
  }
  closedir(bouncedir);
  if (!no)			/* max no of bounces exceeded */
    strerr_die2x(0,INFO,ERR_MAX_BOUNCE);
				/* save bounce */
  if (!stralloc_copys(&fndate,workdir)) die_nomem();
  if (!stralloc_cats(&fndate,"/bounce/d")) die_nomem();
  pos = fndate.len - 1;
  if (!stralloc_catb(&fndate,strnum,fmt_ulong(strnum,when))) die_nomem();
  if (!stralloc_cats(&fndate,".")) die_nomem();
  if (!stralloc_catb(&fndate,strnum,fmt_ulong(strnum,(unsigned long) getpid())))
	 die_nomem();
  if (addrno) {	/* so that pre-VERP bounces make a d... file per address */
		/* for the first one we use the std-style fname */
    if (!stralloc_cats(&fndate,".")) die_nomem();
    if (!stralloc_catb(&fndate,strnum,fmt_ulong(strnum,addrno))) die_nomem();
  }
  addrno++;	/* get ready for next */
  if (!stralloc_0(&fndate)) die_nomem();
  if (!stralloc_copy(&fndatenew,&fndate)) die_nomem();
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
  if (rename(fndatenew.s,fndate.s) == -1)
    strerr_die6sys(111,FATAL,ERR_MOVE,fndatenew.s," to ",fndate.s,": ");
}

void main(argc,argv)
int argc;
char **argv;
{
  char *local;
  char *host;
  char *action;
  int flagdig = 1;
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

  dir = argv[1];
  if (!dir) die_usage();
  if (*dir == '-') {
    if (dir[1] == 'd') {
      flagdig = 2;
    } else if (dir[1] == 'D') {
      flagdig = 0;
    } else
      die_usage();
    dir = argv[2];
    if (!dir) die_usage();
  }
  if (chdir(dir) == -1)
    strerr_die4sys(111,FATAL,ERR_SWITCH,dir,": ");

  sender = env_get("SENDER");
  action = env_get("DEFAULT");
  local = env_get("LOCAL");
  if (!action) strerr_die2x(100,FATAL,ERR_NODEFAULT);

  getconf_line(&outhost,"outhost",1,FATAL,dir);
  getconf_line(&outlocal,"outlocal",1,FATAL,dir);
  workdir = dir;
  /* now see if -digest-return- */
  if (flagdig == 1) {
    flagdig = 0;
    if (str_len(local) >= str_len(action) + 14)
      if (str_start(local + str_len(local) - 14 - str_len(action),"digest-"))
	flagdig = 2;
  }
  if (flagdig) {
    if (!stralloc_copys(&ddir,dir)) die_nomem();
    if (!stralloc_cats(&ddir,"/digest")) die_nomem();
    if (!stralloc_0(&ddir)) die_nomem();
    workdir = ddir.s;
    if (!stralloc_cats(&outlocal,"-digest")) die_nomem();
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
        if (!stralloc_copys(&listaddr,sender)) die_nomem();
        if (!stralloc_append(&listaddr,"@")) die_nomem();
        if (!stralloc_cats(&listaddr,host)) die_nomem();
        if (!stralloc_0(&listaddr)) die_nomem();
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
        if (!stralloc_catb(&tagline,line.s +len,line.len - len -1)) die_nomem();
		/* NOTE: tagline is dirty! We quote it for sql and rely on */
		/* std log clean for maillog */
        break;
      }
    }
		/* feedback ok even if not sub. Will be filtered by subreceipt*/
		/* For instance, main list feedback is ok, but !issub. */
    subreceipt(workdir,msgnum,&tagline,listaddr.s,2,INFO,FATAL);
    closesql();
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
      strerr_die2sys(111,FATAL,ERR_READ_INPUT);
    if (!match) break;
    if (case_startb(line.s,line.len,TXT_TAG)) {
      len = str_len(TXT_TAG);
      if (!stralloc_catb(&tagline,line.s +len,line.len - len -1)) die_nomem();
		/* NOTE: tagline is dirty! We quote it for sql and rely on */
		/* std log clean for maillog */
      break;
    }
  }
  if (seek_begin(0) == -1)
    strerr_die2sys(111,FATAL,ERR_SEEK_INPUT);
  substdio_fdbuf(&ssin,read,0,inbuf,sizeof(inbuf));

  if (*action) {	/* normal bounce */

    if (slurpclose(0,&bounce,1024) == -1) die_msgin();
    i = str_rchr(action,'=');
    if (!stralloc_copyb(&listaddr,action,i)) die_nomem();
    if (action[i]) {
      if (!stralloc_cats(&listaddr,"@")) die_nomem();
      if (!stralloc_cats(&listaddr,action + i + 1)) die_nomem();
    }
    if (!stralloc_0(&listaddr)) die_nomem();
		/* don't check for sub, since issub() doesn't see sublists */
    switch (subreceipt(workdir,msgnum,&tagline,listaddr.s,-1,INFO,FATAL)) {
	case -1: strerr_die2x(0,INFO,ERR_COOKIE);
	case -2: strerr_die2x(0,INFO,ERR_NOT_ACTIVE);
	default: doit(listaddr.s,msgnum,when,&bounce);
    }
    closesql();
    _exit(0);
  }			/* pre-VERP bounce, in QSBMF format */

  flaghaveheader = 0;
  flaghaveintro = 0;

  for (;;) {
    if (!stralloc_copys(&paragraph,"")) die_nomem();
    for (;;) {
      if (getln(&ssin,&line,&match,'\n') == -1)
        strerr_die2sys(111,FATAL,ERR_READ_INPUT);
      if (!match) die_trash();
      if (!stralloc_cat(&paragraph,&line)) die_nomem();
      if (line.len <= 1) break;
    }

    if (!flaghaveheader) {
      if (!stralloc_copy(&header,&paragraph)) die_nomem();
      flaghaveheader = 1;
      continue;
    }

    if (!flaghaveintro) {
      if (paragraph.s[0] == '-' && paragraph.s[1] == '-')
        continue;		/* skip MIME boundary if it exists */
      if (paragraph.len < 15) die_trash();
      if (str_diffn(paragraph.s,"Hi. This is the",15)) die_trash();
      if (!stralloc_copy(&intro,&paragraph)) die_nomem();
      flaghaveintro = 1;
      continue;
    }

    if (paragraph.s[0] == '-')
      break;

    if (paragraph.s[0] == '<') {	/* find address */
      if (!stralloc_copy(&failure,&paragraph)) die_nomem();

      if (!stralloc_copy(&bounce,&header)) die_nomem();
      if (!stralloc_cat(&bounce,&intro)) die_nomem();
      if (!stralloc_cat(&bounce,&failure)) die_nomem();

      i = byte_chr(failure.s,failure.len,'\n');
      if (i < 3) die_trash();

      if (!stralloc_copyb(&listaddr,failure.s + 1,i - 3)) die_nomem();
      if (byte_chr(listaddr.s,listaddr.len,'\0') == listaddr.len) {
        if (!stralloc_0(&listaddr)) die_nomem();
        if (subreceipt(workdir,msgnum,&tagline,listaddr.s,-1,INFO,FATAL) == 0)
	  doit(listaddr.s,msgnum,when,&bounce);
      }
    }
  }
  closesql();
  _exit(0);
}

