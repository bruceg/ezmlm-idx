/*$Id$*/

#include "auto_bin.h"
#include "stralloc.h"
#include "wrap.h"
#include "die.h"
#include "idx.h"

static stralloc path;

void wrap_execbin(const char *program,
	          stralloc *opts,
	          const char *dir)
{
  const char *args[4];
  int i;

  if (!stralloc_copys(&path,auto_bin())) die_nomem();
  if (!stralloc_cats(&path,program)) die_nomem();
  if (!stralloc_0(&path)) die_nomem();
  args[0] = path.s;

  i = 1;
  if (opts && opts->len > 1) {
    if (!stralloc_0(opts)) die_nomem();
    args[i++] = opts->s;
  }

  args[i++] = dir;
  args[i] = 0;

  wrap_execv(args);
}
