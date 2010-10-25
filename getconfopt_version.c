#include "auto_version.h"
#include "getconfopt.h"
#include "strerr.h"

static int parse(struct option *o,const char *arg)
{
  strerr_die2x(0,"ezmlm version: ",auto_version);
  return 0;
  (void)o;
  (void)arg;
}

const struct option_type opt_version = {
  0,
  parse,
  0
};
