#include "die.h"
#include "getconfopt.h"
#include "stralloc.h"

static int copy_flag(struct option *o,const char *arg)
{
  if (!stralloc_append(o->var.str,&o->ch)) die_nomem();
  return 1;
  (void)arg;
}

static void fetch(struct option *o)
{
  (void)o;
}

const struct option_type opt_copy_flag = {
  0,
  copy_flag,
  fetch
};
