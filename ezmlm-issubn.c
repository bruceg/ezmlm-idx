/*$Id$*/

#include <unistd.h>
#include "strerr.h"
#include "env.h"
#include "subscribe.h"
#include "sgetopt.h"
#include "errtxt.h"
#include "die.h"
#include "idx.h"
#include "config.h"
#include "auto_version.h"

const char FATAL[] = "ezmlm-issubn: fatal: ";
const char USAGE[] =
"ezmlm-issubn: usage: ezmlm-issubn [-nN] dir [dir1 ...]";

void main(int argc,char **argv)
{
  char *dir;
  char *addr;
  int flagsub = 0;
  int opt;

  addr = env_get("SENDER");
  if (!addr) die_sender();	/* REQUIRE sender */

  while ((opt = getopt(argc,argv,"nNvV")) != opteof)
    switch(opt) {
      case 'n': flagsub = 99; break;
      case 'N': flagsub = 0; break;
      case 'v':
      case 'V': strerr_die2x(0, "ezmlm-issubn version: ",auto_version);
      default:
	die_usage();
    }

  startup(argv[optind]);

  while ((dir = argv[optind++])) {
    if (dir[0] != '/')
      strerr_die2x(100,FATAL,ERR_SLASH);
    if (issub(dir,0,addr)) {
      closesub();
      _exit(flagsub);		/* subscriber */
    }
  }
  closesub();
  if (flagsub)			/* not subscriber anywhere */
    _exit(0);
  else
    _exit(99);
}
