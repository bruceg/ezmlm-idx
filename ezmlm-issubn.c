#include <unistd.h>
#include "strerr.h"
#include "env.h"
#include "subdb.h"
#include "sgetopt.h"
#include "messages.h"
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
  int senderissub;

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

  if (optind >= argc)
    senderissub = !!issub(0,addr,0);
  else {
    senderissub = 0;
    while ((subdir = argv[optind++]) != 0) {
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
