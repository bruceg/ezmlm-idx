#include "getconf.h"
#include "getconfopt.h"
#include "stralloc.h"

static int parse(struct option *o,const char *arg)
{
  *o->var.cstr = arg;
  return 1;
}

static void fetch(struct option *o)
{
  stralloc s = {0};
  /* This function intentionally never frees the C string pointer
   * created for the stralloc above.  This gives the variable its own
   * copy, preventing reallocation problems. */
  if (getconf_line(&s,o->filename,0))
    *o->var.cstr = s.s;
}

const struct option_type opt_cstr = {
  1,
  parse,
  fetch
};
