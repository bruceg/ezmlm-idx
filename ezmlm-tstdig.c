/*$Id$*/

#include <sys/types.h>
#include "stralloc.h"
#include "substdio.h"
#include "readwrite.h"
#include "strerr.h"
#include "sig.h"
#include "getconf.h"
#include "env.h"
#include "fmt.h"
#include "now.h"
#include "lock.h"
#include "sgetopt.h"
#include "errtxt.h"
#include "idx.h"

#define FATAL "ezmlm-tstdig: fatal: "

void die_usage()
{
  strerr_die1x(100,"ezmlm-tstdig: usage: ezmlm-tstdig [-k kbytes] [-m messages] [-t hours] dir");
}

void die_nomem() { strerr_die2x(111,FATAL,ERR_NOMEM); }

stralloc line = {0};

substdio ssnum;
char numbuf[16];

char strnum[FMT_ULONG];

int flaglocal = 0;

void main(argc,argv)
int argc;
char **argv;
{
  char *dir;
  char *local;
  char *def;
  int opt;
  unsigned int pos;
  unsigned long num, digsize, dignum;
  unsigned long cumsize = 0L;
  unsigned long deltanum = 0L;
  unsigned long deltawhen = 0L;
  unsigned long deltasize = 0L;
  unsigned long when, tsttime, digwhen;
  int fd,fdlock;

  (void) umask(022);
  sig_pipeignore();
  when = (unsigned long) now();

  while ((opt = getopt(argc,argv,"k:t:m:vV")) != opteof)
    switch(opt) {
      case 'k':
                if (optarg)
                  scan_ulong(optarg,&deltasize);
                break;
      case 't':
                if (optarg)	/* hours */
                  scan_ulong(optarg,&deltawhen);
                break;
      case 'm':
                if (optarg)
                  scan_ulong(optarg,&deltanum);
                break;
      case 'v':
      case 'V': strerr_die2x(0,"ezmlm-tstdig version: ",EZIDX_VERSION);
      default:
	die_usage();
    }


  dir = argv[optind++];
  if (!dir) die_usage();

  if (chdir(dir) == -1)
    strerr_die4sys(111,FATAL,ERR_SWITCH,dir,": ");

  if (argv[optind])
    die_usage();	/* avoid common error of putting options after dir */
  if (!getconf_line(&line,"num",0,FATAL,dir))
    _exit(99);		/* no msgs no shirt -> no digest */
  if(!stralloc_0(&line)) die_nomem();
  pos = scan_ulong(line.s,&num);
  if (line.s[pos] == ':')
    scan_ulong(line.s+pos+1,&cumsize);

  if (getconf_line(&line,"dignum",0,FATAL,dir)) {
    if(!stralloc_0(&line)) die_nomem();
    pos = scan_ulong(line.s,&dignum);
    if (line.s[pos] == ':')
      pos += 1 + scan_ulong(line.s+pos+1,&digsize);
    if (line.s[pos] == ':')
      scan_ulong(line.s+pos+1,&digwhen);
  } else {
    dignum = 0L;	/* no file, not done any digest */
    digsize = 0L;	/* nothing digested */
    digwhen = 0L;	/* will force a digest, but the last one was eons  */
			/* ago. ezmlm-get sends it out only if there are   */
			/* messages. This is as it should for new lists.   */
  }
  local = env_get("LOCAL");
  if (local && *local) {			/* in editor or manager */
    def = env_get("DEFAULT");
    if (def && *def) {				/* qmail>=1.02 and manager */
      if (!case_starts(def,"dig") || case_starts(def,"digest-"))
        _exit(0);
    } else {				/* older qmail versions or editor */
      if (!getconf_line(&line,"inlocal",0,FATAL,dir)) {
	flaglocal = 1;
      } else {
	pos = str_len(local);
	if (pos <= line.len) {		/* maybe qmail>=1.02 and editor */
	  flaglocal = 1;		/* editor and qmail>=1.02. No harm */
					/* if we're wrong */
	} else {			/* older qmail */
	  if (case_diffb(local,line.len,line.s))	/* local */
	    flaglocal = 1;		/* minimal harm */
          else if (pos < line.len +4 ||	/* in manager and non-digest */
		!case_starts(local+line.len,"-dig"))
	    _exit(0);
          else if (case_starts(local+line.len,"-digest-"))
	    _exit(0);
	}
      }
    }
  }

  if (!deltawhen && !deltasize && !deltanum) _exit(0);
  if ((deltawhen && ((digwhen + deltawhen * 3600L) <= when)) ||
      (deltasize && ((digsize + (deltasize << 2)) <= cumsize)) ||
      (deltanum && ((dignum + deltanum) <= num))) {	/* digest! */
    if (flaglocal) {	/* avoid multiple digests. Of course, ezmlm-tstdig*/
			/* belongs in ezmlm-digest, but it's too late ....*/
      fdlock = open_append("lock");
      if (fdlock == -1)
        strerr_die2sys(111,FATAL,ERR_OPEN_LOCK);
      if (lock_ex(fdlock) == -1) {
        close(fdlock);
        strerr_die2sys(111,FATAL,ERR_OBTAIN_LOCK);
      }
      getconf_line(&line,"tstdig",0,dir,FATAL);
      if (!stralloc_0(&line)) die_nomem();
      scan_ulong(line.s,&tsttime);	/* give digest 1 h to complete */
					/* nobody does digests more often */
      if ((tsttime + 3600L < when) || (tsttime <= digwhen)) {
        fd = open_trunc("tstdign");
        if (fd == -1)
          strerr_die6sys(111,FATAL,ERR_CREATE,dir,"/","tstdign",": ");
        substdio_fdbuf(&ssnum,write,fd,numbuf,sizeof(numbuf));
        if (substdio_put(&ssnum,strnum,fmt_ulong(strnum,when)) == -1)
          strerr_die6sys(111,FATAL,ERR_WRITE,dir,"/","tstdign",": ");
        if (substdio_puts(&ssnum,"\n") == -1)
          strerr_die6sys(111,FATAL,ERR_WRITE,dir,"/","tstdign",": ");
        if (substdio_flush(&ssnum) == -1)
          strerr_die6sys(111,FATAL,ERR_FLUSH,dir,"/","tstdign",": ");
        if (fsync(fd) == -1)
          strerr_die6sys(111,FATAL,ERR_SYNC,dir,"/","tstdign",": ");
        if (close(fd) == -1)
          strerr_die6sys(111,FATAL,ERR_CLOSE,dir,"/","tstdign",": ");
        if (rename("tstdign","tstdig") == -1)
          strerr_die4sys(111,FATAL,ERR_MOVE,"tstdign",": ");
        _exit(0);
      }
    } else
        _exit(0);
  }
  _exit(99);
}

