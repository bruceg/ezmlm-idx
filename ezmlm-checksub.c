#include <unistd.h>
#include "strerr.h"
#include "env.h"
#include "sender.h"
#include "str.h"
#include "subdb.h"
#include "subfd.h"
#include "substdio.h"
#include "getconfopt.h"
#include "messages.h"
#include "die.h"
#include "idx.h"
#include "config.h"
#include "auto_version.h"

const char FATAL[] = "ezmlm-checksub: fatal: ";
const char USAGE[] =
"ezmlm-checksub: usage: ezmlm-checksub [-m MESSAGE] [-nN] dir [subdir ...]";

static const char *message = 0;
static int flagsub = 1;

static struct option options[] = {
  OPT_CSTR(message,'m',0),
  OPT_FLAG(flagsub,'n',0,0),
  OPT_FLAG(flagsub,'N',1,0),
  OPT_END
};

int main(int argc,char **argv)
{
  const char *subdir;
  const char *addr;
  int senderissub;
  int i;

  addr = get_sender();
  if (!addr) die_sender();	/* REQUIRE sender */

  i = getconfopt(argc,argv,options,1,0);
  initsub(0);
  if (message == 0)
    message = flagsub ? MSG(TXT_ONLY_SUBSCRIBERS) : MSG(TXT_REJECT_POSTS);

  if (i >= argc)
    senderissub = !!issub(0,addr,0);
  else {
    senderissub = 0;
    while ((subdir = argv[i++]) != 0) {
      if (issub(subdir,addr,0)) {
	senderissub = 1;
	break;
      }
    }
  }
  closesub();

  /* senderissub and flagsub are both either 0 or 1 */
  if (senderissub != flagsub) {
    substdio_puts(subfdout,message);
    substdio_putflush(subfdout,"\n",1);
    _exit(100);
  }
  _exit(0);
}
