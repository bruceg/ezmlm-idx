#include "getconf.h"
#include "getconfopt.h"

static int parse(struct option *o,const char *arg)
{
  *o->var.cstr = o->value.cstr;
  return 1;
  (void)arg;
}

static void fetch(struct option *o)
{
  if (getconf_isset(o->filename))
    *o->var.cstr = o->value.cstr;
}

const struct option_type opt_cstr_flag = {
  0,
  parse,
  fetch
};
