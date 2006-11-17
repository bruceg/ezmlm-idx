/*$Id$*/

#include "die.h"
#include "stralloc.h"
#include "sub_std.h"

void std_makepath(stralloc *fn,
		  const char *dir,
		  const char *subdir,
		  const char *append,
		  char ch)
{
  if (!stralloc_copys(fn,dir)) die_nomem();
  if (subdir != 0 && *subdir != 0) {
    if (!stralloc_append(fn,"/")) die_nomem();
    if (!stralloc_cats(fn,subdir)) die_nomem();
  }
  if (!stralloc_cats(fn,append)) die_nomem();
  if (ch > 0)
    if (!stralloc_catb(fn,&ch,1)) die_nomem();
  if (!stralloc_0(fn)) die_nomem();
}
