#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "substdio.h"
#include "stralloc.h"
#include "getln.h"
#include "readwrite.h"
#include "exit.h"
#include "open.h"
#include "error.h"
#include "strerr.h"
#include "byte.h"
#include "scan.h"

stralloc target = {0};
char *to;

const char FATAL[] = "installer: fatal: ";
void nomem() { strerr_die2x(111,FATAL,"out of memory"); }

char inbuf[SUBSTDIO_INSIZE];
char outbuf[SUBSTDIO_OUTSIZE];
substdio ssin;
substdio ssout;

void doit(line)
stralloc *line;
{
  char *x;
  unsigned int xlen;
  unsigned int i;
  char *type;
  char *uidstr;
  char *gidstr;
  char *modestr;
  char *mid;
  char *name;
  unsigned long uid;
  unsigned long gid;
  unsigned long mode;
  int fdin;
  int fdout;
  int opt;

  x = line->s; xlen = line->len;
  opt = (*x == '?');
  x += opt;
  xlen -= opt;

  type = x;
  i = byte_chr(x,xlen,':'); if (i == xlen) return;
  x[i++] = 0; x += i; xlen -= i;

  uidstr = x;
  i = byte_chr(x,xlen,':'); if (i == xlen) return;
  x[i++] = 0; x += i; xlen -= i;

  gidstr = x;
  i = byte_chr(x,xlen,':'); if (i == xlen) return;
  x[i++] = 0; x += i; xlen -= i;

  modestr = x;
  i = byte_chr(x,xlen,':'); if (i == xlen) return;
  x[i++] = 0; x += i; xlen -= i;

  mid = x;
  i = byte_chr(x,xlen,':'); if (i == xlen) return;
  x[i++] = 0; x += i; xlen -= i;

  name = x;
  i = byte_chr(x,xlen,':'); if (i == xlen) return;
  x[i++] = 0; x += i; xlen -= i;

  stralloc_copys(&target,to);
  stralloc_cats(&target,mid);
  stralloc_cats(&target,name);
  stralloc_0(&target);
  if (xlen > 0) name = x;

  uid = -1; if (*uidstr) scan_ulong(uidstr,&uid);
  gid = -1; if (*gidstr) scan_ulong(gidstr,&gid);
  scan_8long(modestr,&mode);

  switch(*type) {
    case 'd':
      if (mkdir(target.s,0700) == -1)
        if (errno != error_exist)
	  strerr_die3sys(111,FATAL,"unable to mkdir ",target.s);
      break;

    case 'c':
      fdin = open_read(name);
      if (fdin == -1) {
	if (opt)
	  return;
	else
	  strerr_die3sys(111,FATAL,"unable to read ",name);
      }
      substdio_fdbuf(&ssin,read,fdin,inbuf,sizeof(inbuf));

      fdout = open_trunc(target.s);
      if (fdout == -1)
	strerr_die3sys(111,FATAL,"unable to write ",target.s);
      substdio_fdbuf(&ssout,write,fdout,outbuf,sizeof(outbuf));

      switch(substdio_copy(&ssout,&ssin)) {
	case -2:
	  strerr_die3sys(111,FATAL,"unable to read ",name);
	case -3:
	  strerr_die3sys(111,FATAL,"unable to write ",target.s);
      }

      close(fdin);
      if (substdio_flush(&ssout) == -1)
	strerr_die3sys(111,FATAL,"unable to write ",target.s);
      if (fsync(fdout) == -1)
	strerr_die3sys(111,FATAL,"unable to write ",target.s);
      close(fdout);
      break;

    default:
      return;
  }

  if (chown(target.s,uid,gid) == -1)
    strerr_die3sys(111,FATAL,"unable to chown ",target.s);
  if (chmod(target.s,mode) == -1)
    strerr_die3sys(111,FATAL,"unable to chmod ",target.s);
}

char buf[256];
substdio in = SUBSTDIO_FDBUF(read,0,buf,sizeof(buf));
stralloc line = {0};

int main(argc,argv)
int argc;
char **argv;
{
  int match;

  umask(077);

  to = argv[1];
  if (!to) strerr_die2x(100,FATAL,"installer: usage: install dir");

  for (;;) {
    if (getln(&in,&line,&match,'\n') == -1)
      strerr_die2sys(111,FATAL,"unable to read input");
    if (line.len > 0)
      line.s[--line.len] = 0;
    doit(&line);
    if (!match)
      _exit(0);
  }
  (void)argc;
}
