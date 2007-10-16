#include "msgtxt.h"
#include "constmap.h"
#include "die.h"
#include "stralloc.h"
#include "getconf.h"
#include "str.h"
#include "strerr.h"

static const char basetxts[] =
"ERR_NOMEM:out of memory\0"
"ERR_READ:unable to read \0"
"ERR_NOEXIST: does not exist\0"
"ERR_SUBST_UNSAFE:ERR_SUBST_UNSAFE\0"
;

static stralloc msgtxt = {0};
static struct constmap msgmap = {0};

void msgtxt_init(void)
{
  if (msgmap.num > 0)
    return;
  if (!constmap_init(&msgmap,basetxts,sizeof basetxts,1))
    strerr_die2x(100,FATAL,"out of memory");
  getconf(&msgtxt,"msgtxt",1);
  constmap_free(&msgmap);
  if (!constmap_init(&msgmap,msgtxt.s,msgtxt.len,1))
    strerr_die2x(100,FATAL,"out of memory");
}

const char *MSG(const char *name)
{
  const char *c;
  return ((c = constmap(&msgmap,name,str_len(name))) != 0) ? c : name;
}
