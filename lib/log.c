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
static char num[FMT_ULONG];
static stralloc line = {0};
static stralloc fn = {0};

void logaddr(const char *subdir,const char *event,
	     const char *addr,const char *comment)
{
  char ch;
  int fd;

  stralloc_copyb(&line,num,fmt_ulong(num,(unsigned long) now()));
  stralloc_cats(&line," ");
  stralloc_cats(&line,event);
  stralloc_cats(&line," ");
  while ((ch = *addr++) != 0) {
    if ((ch < 33) || (ch > 126)) ch = '?';
    stralloc_append(&line,ch);
  }
  if (comment && *comment) {
    stralloc_cats(&line," ");
    while ((ch = *comment++) != 0) {
      if (ch == '\t')
        ch = ' ';
      else 
        if ((ch < 32) || (ch > 126)) ch = '?';
      stralloc_append(&line,ch);
    }
  }
  stralloc_cats(&line,"\n");

  makepath(&fn,subdir,"/Log",0);
  fd = open_append(fn.s);
  if (fd == -1) return;
  substdio_fdbuf(&ss,write,fd,NULL,0);
  substdio_putflush(&ss,line.s,line.len);
  close(fd);
  return;
}
