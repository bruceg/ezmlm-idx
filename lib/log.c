/*$Id$*/

#include <unistd.h>
#include "substdio.h"
#include "readwrite.h"
#include "stralloc.h"
#include "log.h"
#include "now.h"
#include "fmt.h"
#include "open.h"
#include "sub_std.h"

/* appends (not crash-proof) a line to "Log". The format is: */
/* "timestamp event address[ comment]\n". address is free of ' ' */
/* Unprintable chars are changed to '?'. Comment may have spaces */

static substdio ss;
static char buf[1];
static char num[FMT_ULONG];
static stralloc line = {0};
static stralloc fn = {0};

void logaddr(const char *dir,const char *subdir,const char *event,
	     const char *addr,const char *comment)
{
  char ch;
  int fd;

  if (!stralloc_copyb(&line,num,fmt_ulong(num,(unsigned long) now()))) return;
  if (!stralloc_cats(&line," ")) return;
  if (!stralloc_cats(&line,event)) return;
  if (!stralloc_cats(&line," ")) return;
  while ((ch = *addr++) != 0) {
    if ((ch < 33) || (ch > 126)) ch = '?';
    if (!stralloc_append(&line,&ch)) return;
  }
  if (comment && *comment) {
    if (!stralloc_cats(&line," ")) return;
    while ((ch = *comment++) != 0) {
      if (ch == '\t')
        ch = ' ';
      else 
        if ((ch < 32) || (ch > 126)) ch = '?';
      if (!stralloc_append(&line,&ch)) return;
    }
  }
  if (!stralloc_cats(&line,"\n")) return;

  std_makepath(&fn,dir,subdir,"/Log",0);
  fd = open_append(fn.s);
  if (fd == -1) return;
  substdio_fdbuf(&ss,write,fd,buf,sizeof(buf));
  substdio_putflush(&ss,line.s,line.len);
  close(fd);
  return;
}
