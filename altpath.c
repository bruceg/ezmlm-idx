/*$Id$*/

#include "stralloc.h"
#include "config.h"
#include "die.h"
#include "error.h"
#include "errtxt.h"
#include "strerr.h"
#include "open.h"
#include "slurp.h"
#include "altpath.h"

static stralloc path = {0};

const char *altpath(stralloc *s,const char *fn)
{
  if (!stralloc_copy(s,&ezmlmrc)) die_nomem();
  if (!stralloc_append(s,"/")) die_nomem();
  if (!stralloc_cats(s,fn)) die_nomem();
  if (!stralloc_0(s)) die_nomem();
  return s->s;
}

int alt_open_read(const char *fn)
{
  int fd;
  if ((fd = open_read(fn)) == -1) {
    if (errno != error_noent)
      strerr_die4sys(111,FATAL,ERR_OPEN,fn,": ");
    if (ezmlmrc.len != 0) {
      if ((fd = open_read(altpath(&path,fn))) == -1)
	if (errno != error_noent)
	  strerr_die4sys(111,FATAL,ERR_OPEN,path.s,": ");
    }
  }
  if (fd == -1)
    strerr_die4sys(100,FATAL,ERR_OPEN,fn,": ");
  return fd;
}

int alt_slurp(const char *fn,struct stralloc *sa,int bufsize)
{
  switch (slurp(fn,sa,bufsize)) {
  case -1:
    return -1;
  case 0:
    if (ezmlmrc.len != 0)
      return slurp(altpath(&path,fn),sa,bufsize);
    return 0;
  default:
    return 1;
  }
}
