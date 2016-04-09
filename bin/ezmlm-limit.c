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
#include "getconfopt.h"
#include "error.h"
#include "scan.h"
#include "open.h"
#include "messages.h"
#include "die.h"
#include "config.h"
#include "idx.h"

const char FATAL[] = "ezmlm-limit: fatal: ";
const char INFO[] = "ezmlm-limit: info: ";
const char USAGE[] =
"ezmlm-limit: usage: ezmlm-limit [-f file] [-dDF] [-n messages] [-t secs] dir";

static unsigned long deltasecs = LIMSECS;	/* interval to test over (seconds) */
static unsigned long deltanum = LIMMSG;		/* max no messages in interval */
						/* see idx.h. Usually 30 msg/3600 secs*/
static int flagd = 0;				/* =0 create modpost, =1 ignore */
						/* excess, =2 defer excess */
static int flagloop;
static const char *fn = TXT_LOOPNUM;

static struct option options[] = {
  OPT_FLAG(flagd,'d',1,0),
  OPT_FLAG(flagd,'D',0,0),
  OPT_CSTR(fn,'f',0),
  OPT_CSTR_FLAG(fn,'F',TXT_LOOPNUM,0),
  OPT_ULONG(deltanum,'n',0),
  OPT_ULONG(deltasecs,'t',0),
  OPT_END
};

static void die_new(void) { strerr_die2sys(111,FATAL,MSG1(ERR_WRITE,fn)); }

int main(int argc,char **argv)
{
  int opt;
  unsigned int pos;
  unsigned long num, loopnum, when;
  unsigned long loopwhen = 0L;
  int fd;
  stralloc line = {0};

  substdio ssnew;
  char newbuf[16];

  char strnum[FMT_ULONG];

  (void) umask(022);
  sig_pipeignore();
  when = (unsigned long) now();

  opt = getconfopt(argc,argv,options,1,0);

  if (argv[opt])
    die_usage();	/* avoid common error of putting options after dir */
  if (getconf_isset("modpost"))
    _exit(0);		/* already mod */
			/* lock for num and for writing loopnum */
  lockfile("lock");

  if (!getconf_ulong(&num,"num",1))
    _exit(99);						/* no msgs */
  if ((flagloop = getconf_line(&line,fn,0))) {
    stralloc_0(&line);
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
	  strerr_die2sys(111,FATAL,MSG1(ERR_WRITE,"modpost"));
      else {
        close(fd);
        unlink(fn);
        strerr_die2x(0,INFO,MSG(ERR_EXCESS_MOD));
      }
    } else
        strerr_die2x(111,FATAL,MSG(ERR_EXCESS_DEFER));
  }
  _exit(0);
}
