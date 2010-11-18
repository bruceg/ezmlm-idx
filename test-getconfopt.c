#include <sys/types.h>
#include <unistd.h>
#include "fmt.h"
#include "getconfopt.h"
#include "stralloc.h"
#include "substdio.h"

const char FATAL[] = "test-getconfopt: fatal: ";
const char USAGE[] =
"test-getconfopt: usage: test-getconfopt [-aAbBvV] dir";

static int option_a = -1;
static int option_b = -2;
static const char *option_c = 0;
static const char *option_d = 0;
static stralloc option_e;
static stralloc option_f;
static unsigned long option_g = 1;
static unsigned long option_h = 0;

static struct option options[] = {
  OPT_FLAG(option_a,'a',1,"option_a"),
  OPT_FLAG(option_a,'A',0,0),
  OPT_FLAG(option_b,'b',1,0),
  OPT_FLAG(option_b,'B',0,"option_b"),
  OPT_CSTR(option_c,'c',"option_c"),
  OPT_CSTR_FLAG(option_d,'d',"on","option_d"),
  OPT_CSTR_FLAG(option_d,'D',"off",0),
  OPT_COPY_FLAG(option_e,'e'),
  OPT_COPY_FLAG(option_e,'E'),
  OPT_STR(option_f,'f',"option_f"),
  OPT_STR(option_f,'F',"option_F"),
  OPT_ULONG(option_g,'g',"option_g"),
  OPT_ULONG_FLAG(option_h,'h',1,"option_h"),
  OPT_ULONG_FLAG(option_h,'H',0,0),
  OPT_END
};

static substdio ssout;
static char buf[SUBSTDIO_OUTSIZE];

static void put_s(const char *prefix,const char *s)
{
  substdio_puts(&ssout,prefix);
  if (s) {
    substdio_puts(&ssout,"\"");
    substdio_puts(&ssout,s);
    substdio_puts(&ssout,"\"\n");
  }
  else
    substdio_puts(&ssout,"NULL\n");
}

static void put_sa(const char *prefix,const stralloc *sa)
{
  substdio_puts(&ssout,prefix);
  if (sa->s) {
    substdio_puts(&ssout,"\"");
    substdio_put(&ssout,sa->s,sa->len);
    substdio_puts(&ssout,"\"\n");
  }
  else
    substdio_puts(&ssout,"NULL\n");
}

static void put_u(const char *prefix,unsigned long u)
{
  char num[FMT_ULONG];
  substdio_puts(&ssout,prefix);
  substdio_put(&ssout,num,fmt_uint(num,u));
  substdio_puts(&ssout,"\n");
}

int main(int argc,char *argv[])
{
  const char *dir;
  int i;

  i = getconfopt(argc,argv,options,0,&dir);

  substdio_fdbuf(&ssout,write,1,buf,sizeof buf);

  put_u("optind=",optind);
  put_s("dir=",dir);
  put_u("a=",option_a);
  put_u("b=",option_b);
  put_s("c=",option_c);
  put_s("d=",option_d);
  put_sa("e=",&option_e);
  put_sa("f=",&option_f);
  put_u("g=",option_g);
  put_u("h=",option_h);

  substdio_flush(&ssout);
  _exit(0);
}
