#include "die.h"
#include "stralloc.h"
#include "sub_std.h"

void makepath(stralloc *fn,
	      const char *subdir,
	      const char *append,
	      char ch)
{
  stralloc_copys(fn,
                 (subdir != 0
                  && subdir[0] != 0
                  && (subdir[0] != '.' || subdir[1] != 0))
                 ? subdir : ".");
  stralloc_cats(fn,append);
  if (ch > 0)
    stralloc_catb(fn,&ch,1);
  stralloc_0(fn);
}
