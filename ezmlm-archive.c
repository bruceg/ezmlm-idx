#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <unistd.h>
#include "alloc.h"
#include "error.h"
#include "stralloc.h"
#include "str.h"
#include "sig.h"
#include "getconf.h"
#include "strerr.h"
#include "getln.h"
#include "substdio.h"
#include "readwrite.h"
#include "fmt.h"
#include "sgetopt.h"
#include "idxthread.h"
#include "makehash.h"
#include "lock.h"
#include "open.h"
#include "scan.h"
#include "die.h"
#include "wrap.h"
#include "idx.h"
#include "messages.h"
#include "config.h"
#include "auto_version.h"

const char FATAL[] = "ezmlm-archive: fatal: ";
const char USAGE[] =
"ezmlm-archive: usage: ezmlm-archive [-cCFsSTvV] [-f min_msg] [-t max_msg] dir";
const char WARNING[] = "ezmlm-archive: warning: inconsistent index: ";

substdio ssin;
char inbuf[1024];
substdio ssout;
char outbuf[1024];
substdio ssnum;
char numbuf[16];

stralloc line = {0};
stralloc num = {0};
stralloc fn = {0};
stralloc fnn = {0};

char strnum[FMT_ULONG];
int flagerror = 0;
int flagsync = 1;	/* sync() by default, not for -c or -f or -t */
char *dir;

struct ca {
  char *s;		/* start */
  unsigned int l;	/* length */
} ca;

void close_proper(substdio *ss,const char *s,const char *sn)
/* flush,sync,close,move sn->s) */
{
   if (substdio_flush(ss) == -1)
     strerr_die2sys(111,FATAL,MSG1(ERR_FLUSH,s));
  if (flagsync)
    if (fsync(ss->fd) == -1)
       strerr_die2sys(111,FATAL,MSG1(ERR_SYNC,s));
  if (close(ss->fd) == -1)
     strerr_die2sys(111,FATAL,MSG1(ERR_CLOSE,s));
  wrap_rename(sn,s);
}

void write_threads(const msgentry *msgtable,
		   subentry *subtable,
		   const authentry *authtable,
		   const dateentry *datetable,
		   unsigned long from,unsigned long to)
/* Add the current threading data to the thread database without dups */
/* Writes the subject index first, then processes the individual files */
{
  const msgentry *pmsgt;
  subentry *psubt,*psubtm, *psubtlast;
  subentry *presubt = (subentry *)0;
  const authentry *pautht;
  const dateentry *pdatet;
  const char *cp;
  const char *cp1;
  unsigned long msg;
  unsigned long ulmsginthread;
  unsigned long subnum;
  unsigned long authnum;
  unsigned long msgnum;
  unsigned int pos;
  unsigned int startdate,nextdate;
  unsigned int startmsg,nextmsg;
  int fd = -1;
  int fdn = -1;
  int match;
  int ffound;
  int lineno;
  int res;

  psubtm = subtable;		/* now for new threads */
  pdatet = datetable;
  startmsg = nextmsg = 0L;
  nextdate = pdatet->date;
  while (psubtm->sub) {		/* these are in msgnum order */
    if (!presubt)		/* for rewind */
      if (psubtm->lastmsg >= nextmsg)
	presubt = psubtm;	/* this thread extends beyond current month */
    if (psubtm->firstmsg >= nextmsg) {	/* done with this month */
      if (fdn != -1) close_proper(&ssout,fn.s,fnn.s);
      psubtlast = psubtm;		/* last thread done */
      if (presubt)			/* need to rewind? */
	psubtm = presubt;		/* do it */
      psubt = psubtm;			/* tmp pointer to reset done flag */
      presubt = (subentry *)0;		/* reset rewind pointer */
      pdatet++;				/* next month */
      startdate = nextdate;		/* startdate */
      nextdate = pdatet->date;		/* end date */
      startmsg = nextmsg;		/* first message in month */
      nextmsg = pdatet->msg;		/* first message in next month */
      if (!stralloc_copys(&fn,"archive/threads/")) die_nomem();
      if (!stralloc_catb(&fn,strnum,fmt_uint(strnum,startdate))) die_nomem();
      if (!stralloc_copy(&fnn,&fn)) die_nomem();
      if (!stralloc_0(&fn)) die_nomem();
      if (!stralloc_cats(&fnn,"n")) die_nomem();
      if (!stralloc_0(&fnn)) die_nomem();
      if ((fdn = open_trunc(fnn.s)) == -1)
	strerr_die2sys(111,FATAL,MSG1(ERR_CREATE,fnn.s));
      substdio_fdbuf(&ssout,write,fdn,outbuf,sizeof(outbuf));
      if ((fd = open_read(fn.s)) == -1) {
      if (errno != error_noent)
             strerr_die2sys(111,FATAL,MSG1(ERR_OPEN,fn.s));
      } else {
	substdio_fdbuf(&ssin,read,fd,inbuf,sizeof(inbuf));
      for (;;) {
      if (getln(&ssin,&line,&match,'\n') == -1)
             strerr_die2sys(111,FATAL,MSG1(ERR_READ,fn.s));
      if (!match) break;
      pos = scan_ulong(line.s,&msgnum);
      pos++;			/* skip ':' */
      if (msgnum >= from)
	continue;		/* ignore entries from threading range */
      if (line.len < pos + HASHLEN) {
	flagerror = -1;		/* and bad ones */
	continue;
      }
      psubt = subtable;
      cp = line.s + pos;
      ffound = 0;		/* search among already known subjects */
      for (;;) {
	res = str_diffn(psubt->sub,cp,HASHLEN);
	if (res < 0) {
	  if (psubt->higher)
	    psubt = psubt->higher;
	 else
	   break;
	} else if (res > 0) {
	  if (psubt->lower)
	    psubt = psubt->lower;
	  else
	    break;
	} else {
	  ffound = 1;
	  break;
	}
      }
      if (!ffound) {
	if (substdio_put(&ssout,line.s,line.len) == -1)
	  strerr_die2sys(111,FATAL,MSG1(ERR_WRITE,fnn.s));
      } else {			/* new # of msg in thread */
	cp += HASHLEN;		/* HASHLEN [#] Subject always \n at end */
	if (*(cp++) == ' ' && *(cp++) == '[') {
	  cp += scan_ulong(cp,&ulmsginthread);
	  if (*cp == ']') {
	    psubt->msginthread += (unsigned char) (ulmsginthread & 0xff);
	  }
	} else
	  flagerror = -5;
      }
    }
    close(fd);
  }
  continue;
  }

    if (psubtm->firstmsg < nextmsg && psubtm->lastmsg >= startmsg) {
    if (!stralloc_copyb(&line,strnum,fmt_ulong(strnum,psubtm->lastmsg)))
		die_nomem();
    if (!stralloc_cats(&line,":")) die_nomem();
    if (!stralloc_catb(&line,psubtm->sub,HASHLEN)) die_nomem();
    if (!stralloc_cats(&line," [")) die_nomem();
    if (!stralloc_catb(&line,strnum,
	fmt_ulong(strnum,(unsigned long) psubtm->msginthread)))
		die_nomem();
    if (!stralloc_cats(&line,"]")) die_nomem();
    if (!stralloc_catb(&line,psubtm->sub + HASHLEN,psubtm->sublen - HASHLEN))
			 die_nomem();	/* has \n */
    if (substdio_put(&ssout,line.s,line.len) == -1)
	strerr_die2sys(111,FATAL,MSG1(ERR_WRITE,fnn.s));
    }
  psubtm++;
  }
  if (fdn != -1)
    close_proper(&ssout,fn.s,fnn.s);

  psubt = subtable;
  while (psubt->sub) {		/* now the threads */
    if (!stralloc_copys(&fn,"archive/subjects/")) die_nomem();
    if (!stralloc_catb(&fn,psubt->sub,2)) die_nomem();
    if (!stralloc_0(&fn)) die_nomem();
    if (mkdir(fn.s,0755) == -1)
    if (errno != error_exist)
      strerr_die2sys(111,FATAL,MSG1(ERR_CREATE,fn.s));
    fn.s[fn.len - 1] = '/';
    if (!stralloc_catb(&fn,psubt->sub+2,HASHLEN-2)) die_nomem();
    if (!stralloc_copy(&fnn,&fn)) die_nomem();
    if (!stralloc_cats(&fnn,"n")) die_nomem();
    if (!stralloc_0(&fn)) die_nomem();
    if (!stralloc_0(&fnn)) die_nomem();
    if ((fdn = open_trunc(fnn.s)) == -1)
      strerr_die2sys(111,FATAL,MSG1(ERR_CREATE,fnn.s));
    substdio_fdbuf(&ssout,write,fdn,outbuf,sizeof(outbuf));
    if ((fd = open_read(fn.s)) == -1) {
      if (errno != error_noent)
	  strerr_die2sys(111,FATAL,MSG1(ERR_OPEN,fn.s));
      if (substdio_puts(&ssout,psubt->sub) == -1)	/* write subject */
	     strerr_die2sys(111,FATAL,MSG1(ERR_WRITE,fnn.s));
    } else {					/* copy data */
	substdio_fdbuf(&ssin,read,fd,inbuf,sizeof(inbuf));
	lineno = 0;
	for (;;) {
	  if (getln(&ssin,&line,&match,'\n') == -1)
             strerr_die2sys(111,FATAL,MSG1(ERR_READ,fn.s));
          if (!match) break;
	  if (!lineno) {			/* write subject */
	    if (line.len < HASHLEN + 1 || line.s[HASHLEN] != ' ')
		flagerror = -3;
	    if (substdio_put(&ssout,line.s,line.len) == -1)
	       strerr_die2sys(111,FATAL,MSG1(ERR_WRITE,fnn.s));
	    lineno = 1;
	    continue;
	  }
	  (void) scan_ulong(line.s,&msgnum);
	  if (msgnum >= from) break;
	  if (substdio_put(&ssout,line.s,line.len) == -1)
	     strerr_die2sys(111,FATAL,MSG1(ERR_WRITE,fnn.s));
	}
	(void) close(fd);	/* close old index */
      }

    subnum = (unsigned long) (psubt - subtable + 1);	/* idx of this subj */
    pmsgt = msgtable + psubt->firstmsg - from;	/* first message entry */
    for (msg = psubt->firstmsg; msg <= psubt->lastmsg; msg++) {
      if (pmsgt->subnum == subnum) {
        if (!stralloc_copyb(&line,strnum,fmt_ulong(strnum,msg))) die_nomem();
        if (!stralloc_cats(&line,":")) die_nomem();
	if (!stralloc_catb(&line,strnum,fmt_uint(strnum,pmsgt->date)))
		die_nomem();
	if (!stralloc_cats(&line,":")) die_nomem();
        if (pmsgt->authnum) {
	  pautht = authtable + pmsgt->authnum - 1;
	  cp = pautht->auth;
	  cp1 = cp + str_chr(cp,' ');
	  if (cp + HASHLEN != cp1)
	    strerr_die1x(100,MSG(ERR_BAD_INDEX));
	  if (!stralloc_cats(&line,cp))
		die_nomem();				/* hash */
	} else
          if (!stralloc_cats(&line,"\n")) die_nomem();
	if (substdio_put(&ssout,line.s,line.len) == -1)
	  strerr_die2sys(111,FATAL,MSG1(ERR_WRITE,fnn.s));
      }
      pmsgt++;
    }
    close_proper(&ssout,fn.s,fnn.s);
    psubt++;
  }

					/* (no master author index) */
  pautht = authtable;
  while (pautht->auth) {		/* now the authors */
    if (!stralloc_copys(&fn,"archive/authors/")) die_nomem();
    if (!stralloc_catb(&fn,pautht->auth,2)) die_nomem();
    if (!stralloc_0(&fn)) die_nomem();
    if (mkdir(fn.s,0755) == -1)
    if (errno != error_exist)
      strerr_die2sys(111,FATAL,MSG1(ERR_CREATE,fn.s));
    fn.s[fn.len - 1] = '/';
    if (!stralloc_catb(&fn,pautht->auth+2,HASHLEN-2)) die_nomem();
    if (!stralloc_copy(&fnn,&fn)) die_nomem();
    if (!stralloc_cats(&fnn,"n")) die_nomem();
    if (!stralloc_0(&fn)) die_nomem();
    if (!stralloc_0(&fnn)) die_nomem();
    if ((fdn = open_trunc(fnn.s)) == -1)
      strerr_die2sys(111,FATAL,MSG1(ERR_CREATE,fnn.s));
    substdio_fdbuf(&ssout,write,fdn,outbuf,sizeof(outbuf));
      if ((fd = open_read(fn.s)) == -1) {
	if (errno != error_noent)
	  strerr_die2sys(111,FATAL,MSG1(ERR_OPEN,fn.s));
        else {			/* didn't exist before: write author */
          if (substdio_put(&ssout,pautht->auth,pautht->authlen) == -1)
	     strerr_die2sys(111,FATAL,MSG1(ERR_WRITE,fnn.s));
	}
      } else {					/* copy data */
	substdio_fdbuf(&ssin,read,fd,inbuf,sizeof(inbuf));
	lineno = 0;
	for (;;) {
	  if (getln(&ssin,&line,&match,'\n') == -1)
             strerr_die2sys(111,FATAL,MSG1(ERR_READ,fn.s));
          if (!match) break;
	  if (!lineno) {			/* write author */
	    if (line.len < HASHLEN + 1 || line.s[HASHLEN] != ' ')
		flagerror = - 4;
	    if (substdio_put(&ssout,line.s,line.len) == -1)
	       strerr_die2sys(111,FATAL,MSG1(ERR_WRITE,fnn.s));
	    lineno = 1;
	    continue;
	  }
	  (void) scan_ulong(line.s,&msgnum);
	  if (msgnum >= from) break;
	  if (substdio_put(&ssout,line.s,line.len) == -1)
	     strerr_die2sys(111,FATAL,MSG1(ERR_WRITE,fnn.s));
	}
	(void) close(fd);			/* close old index */
      }

    authnum = (unsigned long) (pautht - authtable + 1);	/* idx of this auth */
    pmsgt = msgtable + pautht->firstmsg - from;	/* first message entry */
    for (msg = pautht->firstmsg; msg <= to; msg++) {
      if (pmsgt->authnum == authnum) {
        if (!stralloc_copyb(&line,strnum,fmt_ulong(strnum,msg))) die_nomem();
        if (!stralloc_cats(&line,":")) die_nomem();
	if (!stralloc_catb(&line,strnum,fmt_uint(strnum,pmsgt->date)))
		die_nomem();
	if (!stralloc_cats(&line,":")) die_nomem();
        if (pmsgt->subnum) {
	  psubt = subtable + pmsgt->subnum - 1;
          if (!stralloc_catb(&line,psubt->sub,psubt->sublen))
		die_nomem();
	}
	if (substdio_put(&ssout,line.s,line.len) == -1)
	  strerr_die2sys(111,FATAL,MSG1(ERR_WRITE,fnn.s));
      }
      pmsgt++;
    }
    close_proper(&ssout,fn.s,fnn.s);
    pautht++;
  }
}

int main(int argc,char **argv)
{
  unsigned long archnum = 0L;
  unsigned long to = 0L;
  unsigned long max;
  int fd;
  int fdlock;
  int flagcreate = 0;
  int flagsyncall = 0;
  int opt;
  msgentry *msgtable;
  subentry *subtable;
  authentry *authtable;
  dateentry *datetable;

  (void) umask(022);
  sig_pipeignore();

  while ((opt = getopt(argc,argv,"cCf:FsSt:TvV")) != opteof)
    switch (opt) {
      case 'c':	flagcreate = 1;
		flagsync = 0;
		break;			/* start at beginning of archive */
      case 'C': flagcreate = 0;
		break;	/* Do only archnum+1 => num */
      case 'f': if (optarg) {
		  (void) scan_ulong(optarg,&archnum);
		  archnum = (archnum / 100) * 100;
	        }
		flagsync = 0;
		break;
      case 'F': archnum = 0; break;
      case 's': flagsyncall = 1; break;
      case 'S': flagsyncall = 0; break;
      case 't': if (optarg) {
		  (void) scan_ulong(optarg,&to);
		}
		flagsync = 0;
		break;
      case 'T': to = 0; break;
      case 'v':
      case 'V': strerr_die2x(0,"ezmlm-archive version: ",auto_version);
      default:
        die_usage();
    }

  if (flagsyncall) flagsync = 1;	/* overrides */
  startup(dir = argv[optind++]);

  if (mkdir("archive/threads",0755) == -1)
    if (errno != error_exist)
      strerr_die2sys(111,FATAL,MSG1(ERR_CREATE,"archive/threads"));
  if (mkdir("archive/subjects",0755) == -1)
    if (errno != error_exist)
      strerr_die2sys(111,FATAL,MSG1(ERR_CREATE,"archive/subjects"));
  if (mkdir("archive/authors",0755) == -1)
    if (errno != error_exist)
      strerr_die2sys(111,FATAL,MSG1(ERR_CREATE,"archive/authors"));

	/* Lock list to assure that no ezmlm-send is working on it */
	/* and that the "num" message is final */
  fdlock = lockfile("lock");
					/* get num */
  getconf_ulong(&max,"num",0);
  if (max == 0)
    strerr_die1x(100,MSG(ERR_EMPTY_LIST));
  (void) close(fdlock);

  if (!to || to > max) to = max;

  fdlock = lockfile("archive/lock");	/* lock index */
  if (!flagcreate && !archnum) {	/* adjust archnum (from) / to */
    if (getconf_ulong(&archnum,"archnum",0))
      ++archnum;
  }

  if (archnum > to)
    _exit(0);				/* nothing to do */

					/* do the subject threading */
  idx_mkthreads(&msgtable,&subtable,&authtable,&datetable,archnum,to,max,0);
					/* update the index */
  write_threads(msgtable,subtable,authtable,datetable,archnum,to);
					/* update archnum */
  if ((fd = open_trunc("archnumn")) == -1)
    strerr_die2sys(111,FATAL,MSG1(ERR_CREATE,"archnumn"));
  substdio_fdbuf(&ssnum,write,fd,numbuf,sizeof(numbuf));
  if (substdio_put(&ssnum,strnum,fmt_ulong(strnum,to)) == -1)
     strerr_die2sys(111,FATAL,MSG1(ERR_WRITE,fnn.s));
  if (substdio_puts(&ssnum,"\n") == -1)
     strerr_die2sys(111,FATAL,MSG1(ERR_WRITE,fnn.s));
  close_proper(&ssnum,"archnum","archnumn");
  switch (flagerror) {
    case 0:
       _exit(0);				/* go bye-bye */
    case -1:
       strerr_die2x(99,WARNING,"threads entry with illegal format");
    case -2:
       strerr_die2x(99,WARNING,"thread in index, but threadfile missing");
    case -3:
       strerr_die2x(99,WARNING,"a subject file lacks subject");
    case -4:
       strerr_die2x(99,WARNING,"an author file lacks author/hash");
    case -5:
       strerr_die2x(99,WARNING,"threads entry lacks message count");
    default:
       strerr_die2x(99,WARNING,"something happened that isn't quite right");
  }
}

