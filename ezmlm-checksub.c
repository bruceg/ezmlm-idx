#include <unistd.h>
#include "strerr.h"
#include "env.h"
#include "str.h"
#include "subdb.h"
#include "subfd.h"
#include "substdio.h"
#include "sgetopt.h"
#include "messages.h"
#include "die.h"
#include "idx.h"
#include "config.h"
#include "auto_version.h"

const char FATAL[] = "ezmlm-checksub: fatal: ";
const char USAGE[] =
"ezmlm-checksub: usage: ezmlm-checksub [-m MESSAGE] [-nN] dir [subdir ...]";

void main(int argc,char **argv)
{
  const char *dir;
  const char *subdir;
  const char *addr;
  const char *message = 0;
  int flagsub = 1;
  int opt;
  int senderissub;

  addr = env_get("SENDER");
  if (!addr) die_sender();	/* REQUIRE sender */

  while ((opt = getopt(argc,argv,"m:nNvV")) != opteof)
    switch(opt) {
    case 'm': message = optarg; break;
    case 'n': flagsub = 0; break;
    case 'N': flagsub = 1; break;
    case 'v':
    case 'V': strerr_die2x(0, "ezmlm-checksub version: ",auto_version);
    default:
      die_usage();
    }

  startup(dir = argv[optind++]);
  initsub(0);
  if (message == 0)
    message = flagsub ? MSG(TXT_ONLY_SUBSCRIBERS) : MSG(TXT_REJECT_POSTS);

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

  /* senderissub and flagsub are both either 0 or 1 */
  if (senderissub != flagsub) {
    substdio_puts(subfdout,message);
    substdio_putflush(subfdout,"\n",1);
    _exit(100);
  }
  _exit(0);
}
