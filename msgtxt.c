#include "msgtxt.h"
#include "altpath.h"
#include "config.h"
#include "constmap.h"
#include "copy.h"
#include "die.h"
#include "slurp.h"
#include "stralloc.h"
#include "str.h"
#include "strerr.h"

static const char basetxts[] =
"ERR_NOMEM:out of memory\0"
"ERR_READ:unable to read \0"
"ERR_NOEXIST: does not exist\0"
"ERR_SUBST_UNSAFE:ERR_SUBST_UNSAFE\0"
;

static stralloc msg_local = {0};
static stralloc msg_alt = {0};
static stralloc msg_default = {0};
static struct constmap map_local = {0};
static struct constmap map_alt = {0};
static struct constmap map_default = {0};
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
  switch (slurp(fn,&data,4096)) {
    case -1:
      strerr_die6sys(111,FATAL,ERR_READ,listdir,"/",fn,": ");
    case 0:
      return 0;
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
  int didit;
  if (initialized)
    return;
  didit = readit(&msg_local,"text/messages");
  altpath(&xdata,"text/messages");
  didit += readit(&msg_alt,xdata.s);
  altdefaultpath(&xdata,"text/messages");
  didit += readit(&msg_default,xdata.s);
  if (didit == 0)
    strerr_die2x(100,FATAL,"No file text/messages file could be read");
  constmap_free(&map_local);
  if (!constmap_init(&map_local,msg_local.s,msg_local.len,1)
      || !constmap_init(&map_alt,msg_alt.s,msg_alt.len,1)
      || !constmap_init(&map_default,msg_default.s,msg_default.len,1))
    die_nomem();
  initialized = 1;
}

static const char *MSGn(const char *name,const char *params[10])
{
  const char *c;
  params[0] = name;
  /* Handle messages before config is loaded */
  if (map_local.num == 0)
    constmap_init(&map_local,basetxts,sizeof basetxts,1);
  if ((c = constmap(&map_local,name,str_len(name))) == 0)
    if ((c = constmap(&map_alt,name,str_len(name))) == 0)
      if ((c = constmap(&map_default,name,str_len(name))) == 0)
	c = name;
  if (!stralloc_copys(&data,c)) die_nomem();
  copy_xlate(&xdata,&data,params,'H');
  if (!stralloc_0(&xdata)) die_nomem();
  return xdata.s;
}

const char *MSG(const char *name)
{
  const char *params[10] = {0};
  return MSGn(name,params);
}

const char *MSG1(const char *name,const char *p1)
{
  const char *params[10] = {0};
  params[1] = p1;
  return MSGn(name,params);
}

const char *MSG2(const char *name,const char *p1,const char *p2)
{
  const char *params[10] = {0};
  params[1] = p1;
  params[2] = p2;
  return MSGn(name,params);
}
