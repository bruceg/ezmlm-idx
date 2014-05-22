#include "die.h"
#include "getconf.h"
#include "getconfopt.h"
#include "scan.h"

static int parse(struct option *o,const char *arg)
{
  if (arg[scan_ulong(arg,o->var.ulong)] != 0)
    return 0;
  return 1;
}

static void fetch(struct option *o)
{
  getconf_ulong(o->var.ulong,o->filename,0);
}

const struct option_type opt_ulong = {
  1,
  parse,
  fetch
};
