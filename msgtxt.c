#include "msgtxt.h"
#include "altpath.h"
#include "config.h"
#include "constmap.h"
#include "copy.h"
#include "die.h"
#include "stralloc.h"
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
static int initialized = 0;

static stralloc data = {0};
static stralloc xdata = {0};

static int readit(stralloc *sa,const char *fn)
{
  int i;
  unsigned int j;
  int k;

  if (!stralloc_copys(&data,""))
    die_nomem();
  switch (alt_slurp(fn,&data,128)) {
    case -1:
      strerr_die6sys(111,FATAL,ERR_READ,listdir,"/",fn,": ");
    case 0:
      strerr_die5x(100,FATAL,listdir,"/",fn,ERR_NOEXIST);
  }
  if (!stralloc_append(&data,"\n")) die_nomem();
  if (!stralloc_copys(sa,"")) die_nomem();
  i = 0;
  for (j = 0;j < data.len;++j)
    if (data.s[j] == '\n') {
      k = j;
      if ((k > i) && (data.s[i] != '#')) {
        if (!stralloc_catb(sa,data.s + i,k - i)) die_nomem();
        if (!stralloc_0(sa)) die_nomem();
      }
      i = j + 1;
    }
  return 1;
}

void msgtxt_init(void)
{
  if (initialized)
    return;
  readit(&msgtxt,"text/messages");
  constmap_free(&msgmap);
  if (!constmap_init(&msgmap,msgtxt.s,msgtxt.len,1))
    die_nomem();
  initialized = 1;
}

const char *MSG(const char *name)
{
  const char *c;
  /* Handle messages before config is loaded */
  if (msgmap.num == 0)
    constmap_init(&msgmap,basetxts,sizeof basetxts,1);
  if ((c = constmap(&msgmap,name,str_len(name))) == 0)
    c = name;
  if (!stralloc_copys(&data,c)) die_nomem();
  copy_xlate(&xdata,&data,'H');
  if (!stralloc_0(&xdata)) die_nomem();
  return xdata.s;
}
