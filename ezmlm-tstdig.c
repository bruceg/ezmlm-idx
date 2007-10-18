#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <unistd.h>
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
#include "messages.h"
#include "scan.h"
#include "case.h"
#include "str.h"
#include "open.h"
#include "die.h"
#include "idx.h"
#include "config.h"
#include "auto_version.h"

const char FATAL[] = "ezmlm-tstdig: fatal: ";
const char USAGE[] =
"ezmlm-tstdig: usage: ezmlm-tstdig [-k kbytes] [-m messages] [-t hours] dir";

stralloc line = {0};

substdio ssnum;
char numbuf[16];

char strnum[FMT_ULONG];

int flaglocal = 0;

void main(int argc,char **argv)
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
      case 'V': strerr_die2x(0,"ezmlm-tstdig version: ",auto_version);
      default:
	die_usage();
    }


  startup(dir = argv[optind++]);

  if (argv[optind])
    die_usage();	/* avoid common error of putting options after dir */
  if (!getconf_ulong2(&num,&cumsize,"num",0))
    _exit(99);

  if (getconf_line(&line,"dignum",0)) {
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
    if (def && *def) {				/* in manager */
      if (!case_starts(def,"dig") || case_starts(def,"digest-"))
        _exit(0);
    } else {					/* in editor */
      flaglocal = 1;
    }
  }

  if (!deltawhen && !deltasize && !deltanum) _exit(0);
  if ((deltawhen && ((digwhen + deltawhen * 3600L) <= when)) ||
      (deltasize && ((digsize + (deltasize << 2)) <= cumsize)) ||
      (deltanum && ((dignum + deltanum) <= num))) {	/* digest! */
    if (flaglocal) {	/* avoid multiple digests. Of course, ezmlm-tstdig*/
			/* belongs in ezmlm-digest, but it's too late ....*/
      fdlock = lockfile("lock");
      getconf_line(&line,"tstdig",0);
      if (!stralloc_0(&line)) die_nomem();
      scan_ulong(line.s,&tsttime);	/* give digest 1 h to complete */
					/* nobody does digests more often */
      if ((tsttime + 3600L < when) || (tsttime <= digwhen)) {
        fd = open_trunc("tstdign");
        if (fd == -1)
          strerr_die2sys(111,FATAL,MSG1("ERR_CREATE","tstdign"));
        substdio_fdbuf(&ssnum,write,fd,numbuf,sizeof(numbuf));
        if (substdio_put(&ssnum,strnum,fmt_ulong(strnum,when)) == -1)
          strerr_die2sys(111,FATAL,MSG1("ERR_WRITE","tstdign"));
        if (substdio_puts(&ssnum,"\n") == -1)
          strerr_die2sys(111,FATAL,MSG1("ERR_WRITE","tstdign"));
        if (substdio_flush(&ssnum) == -1)
          strerr_die2sys(111,FATAL,MSG1("ERR_FLUSH","tstdign"));
        if (fsync(fd) == -1)
          strerr_die2sys(111,FATAL,MSG1("ERR_SYNC","tstdign"));
        if (close(fd) == -1)
          strerr_die2sys(111,FATAL,MSG1("ERR_CLOSE","tstdign"));
        if (rename("tstdign","tstdig") == -1)
          strerr_die2sys(111,FATAL,MSG2("ERR_MOVE","tstdign","tstdig"));
        _exit(0);
      }
    } else
        _exit(0);
  }
  _exit(99);
}

