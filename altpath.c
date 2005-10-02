/*$Id$*/

#include "stralloc.h"
#include "config.h"
#include "die.h"
#include "error.h"
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
  if ((fd = open_read(fn)) == -1
      && errno == error_noent
      && ezmlmrc.len > 0)
    fd = open_read(altpath(&path,fn));
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
