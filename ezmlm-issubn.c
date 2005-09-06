/*$Id$*/

#include <unistd.h>
#include "strerr.h"
#include "env.h"
#include "subscribe.h"
#include "sgetopt.h"
#include "errtxt.h"
#include "die.h"
#include "idx.h"
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

  dir = argv[optind++];
  if (chdir(dir) == -1)
    strerr_die4sys(111,FATAL,ERR_SWITCH,dir,": ");

  if (optind >= argc) {
    if (issub(dir,0,addr)) {
      closesub();
      _exit(flagsub);
    }
  }
  else {
    while ((subdir = argv[optind++])) {
      if (issub(dir,subdir,addr)) {
	closesub();
	_exit(flagsub);
      }
    }
  }
  closesub();
  if (flagsub)			/* not subscriber anywhere */
    _exit(0);
  else
    _exit(99);
}
