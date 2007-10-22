#include "messages.h"
#include "altpath.h"
#include "config.h"
#include "constmap.h"
#include "copy.h"
#include "die.h"
#include "slurp.h"
#include "stralloc.h"
#include "str.h"
#include "strerr.h"

#define SPLIT ':'

struct messages
{
  stralloc text;
  struct constmap map;
};

static const char internal[] =
#include "messages-txt.c"
;

static struct messages msg_internal = {
  { (char*)internal, sizeof internal, sizeof internal },
  {0}
};
static struct messages msg_local;
static struct messages msg_alt;
static struct messages msg_default;
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
      strerr_die2sys(111,FATAL,MSG1(ERR_READ,fn));
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

static void init_map(struct messages *m)
{
  if (!constmap_init(&m->map,m->text.s,m->text.len,SPLIT))
    die_nomem();
}

static void init(void)
{
  if (initialized)
    return;
  init_map(&msg_internal);
  initialized = 1;

  readit(&msg_local.text,"text/messages");
  init_map(&msg_local);

  altpath(&xdata,"text/messages");
  readit(&msg_alt.text,xdata.s);
  init_map(&msg_alt);

  altdefaultpath(&xdata,"text/messages");
  readit(&msg_default.text,xdata.s);
  init_map(&msg_default);
}

const char *messages_getn(const char *msg,const char *params[10])
{
  const char *xmsg;
  int msg_len;
  params[0] = msg;

  init();

  msg_len = str_len(msg);
  if (msg_local.map.num == 0
      || (xmsg = constmap(&msg_local.map,msg,msg_len)) == 0)
    if (msg_alt.map.num == 0
	|| (xmsg = constmap(&msg_alt.map,msg,msg_len)) == 0)
      if (msg_default.map.num == 0
	  || (xmsg = constmap(&msg_default.map,msg,msg_len)) == 0)
	if ((xmsg = constmap(&msg_internal.map,msg,msg_len)) == 0)
	  xmsg = msg;

  if (!stralloc_copys(&data,xmsg)) die_nomem();
  copy_xlate(&xdata,&data,params,'H');
  if (!stralloc_0(&xdata)) die_nomem();
  return xdata.s;
}

const char *messages_get(const char *msg,const char *p1,const char *p2)
{
  const char *params[10] = {0};
  params[1] = p1;
  params[2] = p2;
  return messages_getn(msg,params);
}
