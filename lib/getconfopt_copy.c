#include "die.h"
#include "getconfopt.h"
#include "stralloc.h"

static int copy_flag(struct option *o,const char *arg)
{
  stralloc_append(o->var.str,o->ch);
  return 1;
  (void)arg;
}

const struct option_type opt_copy_flag = {
  0,
  copy_flag,
  0
};
