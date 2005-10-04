/*$Id$*/

#include "stralloc.h"
#include "config.h"
#include "die.h"
#include "error.h"
#include "open.h"
#include "slurp.h"
#include "altpath.h"
#include "idx.h"
#include "auto_etc.h"

static stralloc path = {0};

const char *altpath(stralloc *s,const char *fn)
{
  if (!stralloc_copy(s,&ezmlmrc)) die_nomem();
  if (!stralloc_append(s,"/")) die_nomem();
  if (!stralloc_cats(s,fn)) die_nomem();
  if (!stralloc_0(s)) die_nomem();
  return s->s;
}

const char *altdefaultpath(stralloc *s,const char *fn)
{
  if (!stralloc_copys(s,auto_etc)) die_nomem();
  if (!stralloc_cats(s,TXT_DEFAULT)) die_nomem();
  if (!stralloc_append(s,"/")) die_nomem();
  if (!stralloc_cats(s,fn)) die_nomem();
  if (!stralloc_0(s)) die_nomem();
  return s->s;
}

int alt_open_read(const char *fn)
{
  int fd;
  if ((fd = open_read(fn)) == -1
      && errno == error_noent) {
    if (ezmlmrc.len > 0)
      fd = open_read(altpath(&path,fn));
    if (fd == -1
	&& errno == error_noent)
      fd = open_read(altdefaultpath(&path,fn));
  }
  return fd;
}

int alt_slurp(const char *fn,struct stralloc *sa,int bufsize)
{
  int r;
  if ((r = slurp(fn,sa,bufsize)) == 0) {
    if (ezmlmrc.len > 0)
      r = slurp(altpath(&path,fn),sa,bufsize);
    if (r == 0)
      r = slurp(altdefaultpath(&path,fn),sa,bufsize);
  }
  return r;
}
