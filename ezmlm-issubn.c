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
"ezmlm-issubn: usage: ezmlm-issubn [-nN] dir [subdir ...]";

void main(int argc,char **argv)
{
  const char *dir;
  const char *subdir;
  const char *addr;
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

  startup(dir = argv[optind++]);
  initsub(0);

  if (optind >= argc) {
    if (issub(0,addr,0)) {
      closesub();
      _exit(flagsub);
    }
  }
  else {
    /* If all the options are absolute, scan each one; else treat the
     * first option as the base directory and the rest as database
     * names. */
    for (opt = --optind; opt < argc; ++opt) {
      if (argv[opt][0] != '/') {
	++optind;
	break;
      }
    }
    while ((subdir = argv[optind++]) != 0) {
      if (issub(subdir,addr,0)) {
	closesub();
	_exit(flagsub);
      }
    }
  }
  closesub();
  _exit(flagsub ? 0 : 99);		/* not subscriber anywhere */
}
