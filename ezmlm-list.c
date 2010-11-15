#include <unistd.h>
#include "strerr.h"
#include "readwrite.h"
#include "substdio.h"
#include "subfd.h"
#include "subdb.h"
#include "exit.h"
#include "fmt.h"
#include "getconfopt.h"
#include "messages.h"
#include "die.h"
#include "idx.h"
#include "config.h"
#include "auto_version.h"

const char FATAL[] = "ezmlm-list: fatal: ";
const char USAGE[] =
"ezmlm-list: usage: ezmlm-list [-mMnNvV] dir [subdir]";

static int flagnumber = 0; /* default list subscribers, not number of */
static const char *flagsubdb = 0;

static struct option options[] = {
  OPT_CSTR_FLAG(flagsubdb,'m',0,0),
  OPT_CSTR_FLAG(flagsubdb,'M',"std",0),
  OPT_CSTR(flagsubdb,'S',0),
  OPT_FLAG(flagnumber,'n',1,0),
  OPT_FLAG(flagnumber,'N',0,0),
  OPT_END
};

char strnum[FMT_ULONG];

static void die_write(void)
{
  strerr_die2sys(111,FATAL,MSG(ERR_WRITE_STDOUT));
}

int subwrite(const char *s,unsigned int l)
{
  return substdio_put(subfdout,s,l) | substdio_put(subfdout,"\n",1);
}

int dummywrite(const char *s,unsigned int l)
{
  return (int) l;
  (void)s;
}

int main(int argc,char **argv)
{
  const char *subdir;
  unsigned long n;
  int i;

  i = getconfopt(argc,argv,options,1,0);
  initsub(flagsubdb);
  subdir = argv[i];

  if (flagnumber) {
    n = putsubs(subdir,0L,52L,dummywrite);
    if (substdio_put(subfdout,strnum,fmt_ulong(strnum,n)) == -1) die_write();
    if (substdio_put(subfdout,"\n",1) == -1) die_write();
  } else
    (void) putsubs(subdir,0L,52L,subwrite);
  if (substdio_flush(subfdout) == -1) die_write();
  closesub();
  _exit(0);
}
