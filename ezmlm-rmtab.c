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

const char FATAL[] = "ezmlm-rmtab: fatal: ";
const char USAGE[] =
"ezmlm-rmtab: usage: ezmlm-rmtab [-mM] [-S subdb | dir]";

static const char *flagsubdb = 0;

static struct option options[] = {
  OPT_CSTR_FLAG(flagsubdb,'m',0,0),
  OPT_CSTR_FLAG(flagsubdb,'M',"std",0),
  OPT_CSTR(flagsubdb,'S',0),
  OPT_END
};

int main(int argc,char **argv)
{
  const char *dir;
  const char *r;

  getconfopt(argc,argv,options,-1,&dir);
  if (dir == 0 && flagsubdb == 0)
    strerr_die2x(100,FATAL,"must specify either -S or dir");

  initsub(flagsubdb);
  if ((r = rmtab()) != 0)
    strerr_die3x(100,FATAL,"could not remove tables: ",r);
  _exit(0);
}
