/*$Id$*/

#include <unistd.h>
#include "strerr.h"
#include "readwrite.h"
#include "substdio.h"
#include "subfd.h"
#include "subdb.h"
#include "exit.h"
#include "fmt.h"
#include "sgetopt.h"
#include "errtxt.h"
#include "die.h"
#include "idx.h"
#include "config.h"
#include "auto_version.h"

const char FATAL[] = "ezmlm-rmtab: fatal: ";
const char USAGE[] =
"ezmlm-rmtab: usage: ezmlm-rmtab [-mM] [-S subdb | dir]";

void main(int argc,char **argv)
{
  const char *dir;
  const char *flagsubdb = 0;
  int opt;
  const char *r;

  while ((opt = getopt(argc,argv,"mMS:vV")) != opteof)
    switch(opt) {
      case 'm': flagsubdb = 0; break;
      case 'M': flagsubdb = "std"; break;
      case 'S': flagsubdb = optarg; break;
      case 'v':
      case 'V': strerr_die2x(0, "ezmlm-rmtab version: ",auto_version);
      default:
	die_usage();
    }

  if ((dir = argv[optind++]) != 0)
    startup(dir);
  else if (flagsubdb == 0)
    strerr_die2x(100,FATAL,"must specify either -S or dir");

  initsub(flagsubdb);
  if ((r = rmtab()) != 0)
    strerr_die3x(100,FATAL,"could not remove tables: ",r);
  _exit(0);
}
