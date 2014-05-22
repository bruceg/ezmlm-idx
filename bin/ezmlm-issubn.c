#include <unistd.h>
#include "strerr.h"
#include "env.h"
#include "sender.h"
#include "subdb.h"
#include "getconfopt.h"
#include "messages.h"
#include "die.h"
#include "idx.h"
#include "config.h"
#include "auto_version.h"

const char FATAL[] = "ezmlm-issubn: fatal: ";
const char USAGE[] =
"ezmlm-issubn: usage: ezmlm-issubn [-nN] dir [subdir ...]";

static int flagsub = 0;

static struct option options[] = {
  OPT_FLAG(flagsub,'n',99,0),
  OPT_FLAG(flagsub,'N',0,0),
  OPT_END
};

int main(int argc,char **argv)
{
  const char *subdir;
  const char *addr;
  int opt;
  int senderissub;
  int i;

  addr = get_sender();
  if (!addr) die_sender();	/* REQUIRE sender */

  opt = getconfopt(argc,argv,options,1,0);
  initsub(0);

  if (opt >= argc)
    senderissub = !!issub(0,addr,0);
  else {
    /* If all the options are absolute, scan each one; else treat the
     * first option as the base directory and the rest as database
     * names. */
    for (i = --opt; i < argc; ++i) {
      if (argv[i][0] != '/') {
        ++opt;
	break;
      }
    }
    senderissub = 0;
    while ((subdir = argv[opt++]) != 0) {
      if (issub(subdir,addr,0)) {
	senderissub = 1;
	break;
      }
    }
  }
  closesub();
  _exit(senderissub ? flagsub	/* is a subscriber */
	: flagsub ? 0 : 99);	/* not subscriber anywhere */
}
