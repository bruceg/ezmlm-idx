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
#include "errtxt.h"
#include "die.h"
#include "fmt.h"
#include "getln.h"
#include "byte.h"
#include "idx.h"
#include "sgetopt.h"
#include "auto_version.h"

const char FATAL[] = "ezmlm-import: fatal: ";
const char USAGE[] =
"ezmlm-import: usage: ezmlm-import dir mbox";

stralloc fnadir = {0};
stralloc fnaf = {0};
stralloc line = {0};
substdio ssarchive;
char archivebuf[4096];
substdio ssin;
char inputbuf[4096];
unsigned long msgnum;
unsigned long cumsize;
char strnum[FMT_ULONG*2];

int openone(unsigned long outnum)
{
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
      strerr_die4sys(111,FATAL,ERR_CREATE,fnadir.s,": ");
  if ((fd = open_trunc(fnaf.s)) == -1)
    strerr_die4sys(111,FATAL,ERR_WRITE,fnaf.s,": ");

  substdio_fdbuf(&ssarchive,write,fd,archivebuf,sizeof archivebuf);
  return fd;
}

void numwrite(void)
{
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
    strerr_die3sys(111,FATAL,ERR_CREATE,"numnew: ");
  if (rename("numnew","num") == -1)
    strerr_die3sys(111,FATAL,ERR_MOVE,"numnew: ");
  
}

int main(int argc,char *argv[])
{
  const char* dir;
  int fd;
  int match;
  unsigned long msgsize = 0L;
  int opt;
  
  while ((opt = getopt(argc,argv,"vV")) != opteof) {
    switch(opt) {
    case 'v':
    case 'V':
      strerr_die2x(0, "ezmlm-import version: ",auto_version);
    default:
      die_usage();
    }
  }

  if (argc - optind != 2)
    die_usage();

  if ((fd = open_read(argv[optind+1])) == -1)
    strerr_die4sys(111,FATAL,ERR_OPEN,argv[optind+1],": ");
  substdio_fdbuf(&ssin,read,fd,inputbuf,sizeof inputbuf);

  startup(dir = argv[optind]);
  lockfile("lock");

  getconf_ulong2(&msgnum,&cumsize,"num",0);
  
  fd = 0;
  while (getln(&ssin,&line,&match,'\n') == 0 && match) {
    if (line.len > 5
	&& byte_diff(line.s,5,"From ") == 0) {
      if (fd > 0) {
	if (substdio_flush(&ssarchive) == -1
	    || fchmod(fd,MODE_ARCHIVE|0700) == -1
	    || close(fd) == -1)
	  strerr_die4sys(111,FATAL,ERR_WRITE,fnaf.s,": ");
	fd = 0;
      }
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

  numwrite();
  
  return 0;
}
