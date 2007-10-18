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

#define SPLIT '|'

struct messages
{
  stralloc text;
  struct constmap map;
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

void messages_init(void)
{
  if (initialized)
    return;

  readit(&msg_local.text,"text/messages");
  if (!constmap_init(&msg_local.map,msg_local.text.s,msg_local.text.len,SPLIT))
    die_nomem();

  altpath(&xdata,"text/messages");
  readit(&msg_alt.text,xdata.s);
  if (!constmap_init(&msg_alt.map,msg_alt.text.s,msg_alt.text.len,SPLIT))
    die_nomem();

  altdefaultpath(&xdata,"text/messages");
  readit(&msg_default.text,xdata.s);
  if (!constmap_init(&msg_default.map,msg_default.text.s,msg_default.text.len,SPLIT))
    die_nomem();

  initialized = 1;
}

static const char *MSGn(const char *msg,const char *params[10])
{
  const char *xmsg;
  int msg_len;
  params[0] = msg;

  if (!initialized)
    return msg;

  msg_len = str_len(msg);
  if ((xmsg = constmap(&msg_local.map,msg,msg_len)) == 0)
    if ((xmsg = constmap(&msg_alt.map,msg,msg_len)) == 0)
      if ((xmsg = constmap(&msg_default.map,msg,msg_len)) == 0)
	xmsg = msg;

  if (!stralloc_copys(&data,xmsg)) die_nomem();
  copy_xlate(&xdata,&data,params,'H');
  if (!stralloc_0(&xdata)) die_nomem();
  return xdata.s;
}

const char *MSG(const char *msg)
{
  const char *params[10] = {0};
  return MSGn(msg,params);
}

const char *MSG1(const char *msg,const char *p1)
{
  const char *params[10] = {0};
  params[1] = p1;
  return MSGn(msg,params);
}

const char *MSG2(const char *msg,const char *p1,const char *p2)
{
  const char *params[10] = {0};
  params[1] = p1;
  params[2] = p2;
  return MSGn(msg,params);
}
