/*$Id$*/

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "stralloc.h"
#include "substdio.h"
#include "readwrite.h"
#include "strerr.h"
#include "sig.h"
#include "lock.h"
#include "getconf.h"
#include "fmt.h"
#include "now.h"
#include "sgetopt.h"
#include "error.h"
#include "scan.h"
#include "open.h"
#include "errtxt.h"
#include "die.h"
#include "config.h"
#include "idx.h"

const char FATAL[] = "ezmlm-limit: fatal: ";
const char INFO[] = "ezmlm-limit: info: ";
const char USAGE[] =
"ezmlm-limit: usage: ezmlm-limit [-f file] [-dDF] [-n messages] [-t secs] dir";

unsigned long deltasecs = LIMSECS;	/* interval to test over (seconds) */
unsigned long deltanum = LIMMSG;	/* max no messages in interval */
					/* see idx.h. Usually 30 msg/3600 secs*/
int flagd = 0;				/* =0 create modpost, =1 ignore */
					/* excess, =2 defer excess */
int flagmod;				/* list moderated */
int flagloop;
const char *fn = TXT_LOOPNUM;

void die_new(void) { strerr_die4sys(111,FATAL,ERR_WRITE,fn,": "); }

stralloc line = {0};

substdio ssnew;
char newbuf[16];

char strnum[FMT_ULONG];

void main(int argc,char **argv)
{
  char *dir;
  int opt;
  unsigned int pos;
  unsigned long num, loopnum, when;
  unsigned long loopwhen = 0L;
  int fd,fdlock;

  (void) umask(022);
  sig_pipeignore();
  when = (unsigned long) now();

  while ((opt = getopt(argc,argv,"dDf:Fn:t:")) != opteof)
    switch(opt) {
      case 'd': flagd = 1; break;
      case 'D': flagd = 0; break;
      case 'f': if (optarg && *optarg) fn = optarg; break;
      case 'F': fn = TXT_LOOPNUM;
      case 'n':
                if (optarg)
                  scan_ulong(optarg,&deltanum);
                break;
      case 't':
                if (optarg)
                  scan_ulong(optarg,&deltasecs);
                break;
      default:
	die_usage();
  }

  startup(dir = argv[optind++]);

  if (argv[optind])
    die_usage();	/* avoid common error of putting options after dir */
  if ((flagmod = getconf_line(&line,"modpost",0,dir)))
    _exit(0);		/* already mod */
			/* lock for num and for writing loopnum */
  fdlock = lockfile("lock");

  if (!getconf_ulong(&num,"num",1,dir))
    _exit(99);						/* no msgs */
  if ((flagloop = getconf_line(&line,fn,0,dir))) {
    if(!stralloc_0(&line)) die_nomem();
    pos = scan_ulong(line.s,&loopnum);			/* msg when written */
    if (line.s[pos] == ':')
      scan_ulong(line.s+pos+1,&loopwhen);		/* time written */
  }
  if (!flagloop || loopwhen + deltasecs < when || loopwhen > when) {
					/* loopnum too old, bad or not there */
      fd = open_trunc(fn);		/* no need to write crash-proof */
      if (fd == -1) die_new();
      substdio_fdbuf(&ssnew,write,fd,newbuf,sizeof(newbuf));
      if (substdio_put(&ssnew,strnum,fmt_ulong(strnum,num)) == -1) die_new();
      if (substdio_puts(&ssnew,":") == -1) die_new();
      if (substdio_put(&ssnew,strnum,fmt_ulong(strnum,when)) == -1) die_new();
      if (substdio_puts(&ssnew,"\n") == -1) die_new();
      if (substdio_flush(&ssnew) == -1) die_new();
      close(fd);
  } else if (num >= loopnum + deltanum) {	/* excess messages */
    if (!flagd) {
      if ((fd = open_append("modpost")) == -1)	/* create dir/modpost */
	  strerr_die3sys(111,FATAL,ERR_WRITE,"subpost:");
      else {
        close(fd);
        unlink(fn);
        strerr_die2x(0,INFO,ERR_EXCESS_MOD);
      }
    } else
        strerr_die2x(111,FATAL,ERR_EXCESS_DEFER);
  }
  _exit(0);
}
