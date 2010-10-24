#include "getconf.h"
#include "getconfopt.h"

static int parse(struct option *o,const char *arg)
{
  *o->var.flag = o->value.flag;
  return 1;
  (void)arg;
}

static void fetch(struct option *o)
{
  *o->var.flag = !!getconf_isset(o->filename) ^ !o->value.flag;
}

const struct option_type opt_flag = {
  0,
  parse,
  fetch
};
