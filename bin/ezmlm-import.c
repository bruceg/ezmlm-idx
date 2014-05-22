#include <sys/types.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include "stralloc.h"
#include "getconf.h"
#include "config.h"
#include "error.h"
#include "strerr.h"
#include "substdio.h"
#include "lock.h"
#include "open.h"
#include "messages.h"
#include "die.h"
#include "wrap.h"
#include "fmt.h"
#include "getln.h"
#include "byte.h"
#include "idx.h"
#include "getconfopt.h"
#include "auto_version.h"

const char FATAL[] = "ezmlm-import: fatal: ";
const char USAGE[] =
"ezmlm-import: usage: ezmlm-import dir [/path/to/mbox]";

static struct option options[] = {
  OPT_END
};

static stralloc fnaf = {0};
static substdio ssarchive;
static char archivebuf[4096];

static int openone(unsigned long outnum)
{
  static stralloc fnadir = {0};
  char strnum[FMT_ULONG];
  int fd;
  if (!stralloc_copys(&fnadir,"archive/")) die_nomem();
  if (!stralloc_catb(&fnadir,strnum,
		     fmt_ulong(strnum,outnum / 100))) die_nomem();
  if (!stralloc_copy(&fnaf,&fnadir)) die_nomem();
  if (!stralloc_cats(&fnaf,"/")) die_nomem();
  if (!stralloc_catb(&fnaf,strnum,
		     fmt_uint0(strnum,(unsigned int)(outnum % 100),2)))
    die_nomem();
  if (!stralloc_0(&fnadir)) die_nomem();
  if (!stralloc_0(&fnaf)) die_nomem();

  if (mkdir(fnadir.s,0755) == -1)
    if (errno != error_exist)
      strerr_die2sys(111,FATAL,MSG1(ERR_CREATE,fnadir.s));
  if ((fd = open_trunc(fnaf.s)) == -1)
    strerr_die2sys(111,FATAL,MSG1(ERR_WRITE,fnaf.s));

  substdio_fdbuf(&ssarchive,write,fd,archivebuf,sizeof archivebuf);
  return fd;
}

static void numwrite(unsigned long msgnum,unsigned long cumsize)
{
  char strnum[FMT_ULONG*2];
  int fd;
  int i;

  i = fmt_ulong(strnum,msgnum);
  strnum[i++] = ':';
  i += fmt_ulong(strnum+i,cumsize);
  strnum[i++] = '\n';

  if ((fd = open_trunc("numnew")) == -1
      || write(fd, strnum, i) != i
      || fsync(fd) == -1
      || close(fd) == -1)
    strerr_die2sys(111,FATAL,MSG1(ERR_CREATE,"numnew"));
  wrap_rename("numnew","num");
}

static void flushit(int fd)
{
  if (fd > 0)
    if (substdio_flush(&ssarchive) == -1
	|| fchmod(fd,MODE_ARCHIVE|0700) == -1
	|| close(fd) == -1)
      strerr_die2sys(111,FATAL,MSG1(ERR_WRITE,fnaf.s));
}

int main(int argc,char *argv[])
{
  int fd;
  int match;
  unsigned long msgsize = 0L;
  int opt;
  stralloc line = {0};
  substdio ssin;
  char inputbuf[4096];
  unsigned long msgnum;
  unsigned long cumsize;


  opt = getconfopt(argc,argv,options,1,0);
  switch (argc - opt) {
    case 0:
      substdio_fdbuf(&ssin,read,0,inputbuf,sizeof inputbuf);
      break;
    case 1:
      if ((fd = open_read(argv[opt])) == -1)
        strerr_die2sys(111,FATAL,MSG1(ERR_OPEN,argv[opt]));
      substdio_fdbuf(&ssin,read,fd,inputbuf,sizeof inputbuf);
      break;
    default:
      die_usage();
  }

  lockfile("lock");

  getconf_ulong2(&msgnum,&cumsize,"num",0);
  
  fd = 0;
  while (getln(&ssin,&line,&match,'\n') == 0 && match) {
    if (line.len > 5
	&& byte_diff(line.s,5,"From ") == 0) {
      flushit(fd);
      ++msgnum;
      cumsize += (msgsize + 128L) >> 8;
      msgsize = 0L;
      fd = openone(msgnum);
    }
    else if (fd > 0) {
      substdio_put(&ssarchive,line.s,line.len);
      msgsize += line.len;
    }
  }
  cumsize += (msgsize + 128L) >> 8;

  flushit(fd);

  numwrite(msgnum,cumsize);
  
  return 0;
}
