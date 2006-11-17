/*$Id$*/

#include <unistd.h>
#include "strerr.h"
#include "readwrite.h"
#include "substdio.h"
#include "subfd.h"
#include "subscribe.h"
#include "exit.h"
#include "fmt.h"
#include "sgetopt.h"
#include "errtxt.h"
#include "die.h"
#include "idx.h"
#include "config.h"
#include "auto_version.h"

const char FATAL[] = "ezmlm-list: fatal: ";
const char USAGE[] =
"ezmlm-list: usage: ezmlm-list [-mMnNvV] dir [subdir]";

int flagnumber = 0;	/* default list subscribers, not number of */

char strnum[FMT_ULONG];

static void die_write(void)
{
  strerr_die3sys(111,FATAL,ERR_WRITE,"stdout: ");
}

int subwrite(const char *s,unsigned int l)
{
  return substdio_put(subfdout,s,l) | substdio_put(subfdout,"\n",1);
}

int dummywrite(const char *s,unsigned int l)
{
  return (int) l;
  (void)s;
}

void main(int argc,char **argv)
{
  const char *dir;
  const char *subdir;
  int flagmysql = 1;	/* use if supported */
  unsigned long n;
  int opt;

  while ((opt = getopt(argc,argv,"mMnNvV")) != opteof)
    switch(opt) {
      case 'm': flagmysql = 1; break;
      case 'M': flagmysql = 0; break;
      case 'n': flagnumber = 1; break;
      case 'N': flagnumber = 0; break;
      case 'v':
      case 'V': strerr_die2x(0, "ezmlm-list version: ",auto_version);
      default:
	die_usage();
    }

  startup(dir = argv[optind++]);
  initsub(dir);
  subdir = argv[optind];

  if (flagnumber) {
    n = putsubs(dir,subdir,0L,52L,dummywrite,flagmysql);
    if (substdio_put(subfdout,strnum,fmt_ulong(strnum,n)) == -1) die_write();
    if (substdio_put(subfdout,"\n",1) == -1) die_write();
  } else
    (void) putsubs(dir,subdir,0L,52L,subwrite,flagmysql);
  if (substdio_flush(subfdout) == -1) die_write();
  closesub();
  _exit(0);
}
