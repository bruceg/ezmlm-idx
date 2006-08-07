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

static int flagsub = 0;
const char *addr;

static void testsub(const char *dir,const char *subdir)
{
  if (issub(dir,subdir,addr)) {
    closesub();
    _exit(flagsub);
  }
}

void main(int argc,char **argv)
{
  const char *dir;
  const char *subdir;
  const char *prevsubdir;
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
  prevsubdir = 0;

  /* This is somewhat convoluted logic to handle both cases where all
   * command-lne arguments are absolute and where some are relative. 
   * In the latter case, do not check the primary database. */
  while ((subdir = argv[optind++]) != 0) {
    if (subdir[0] == '/') {
      if (prevsubdir == 0)
	testsub(dir,0);
      dir = subdir;
      prevsubdir = 0;
    }
    else {
      testsub(dir,subdir);
      prevsubdir = subdir;
    }
  }
  if (prevsubdir == 0)
    testsub(dir,0);
  closesub();
  if (flagsub)			/* not subscriber anywhere */
    _exit(0);
  else
    _exit(99);
}
