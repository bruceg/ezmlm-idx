/*$Id$*/
#include "strerr.h"
#include "readwrite.h"
#include "substdio.h"
#include "subscribe.h"
#include "exit.h"
#include "fmt.h"
#include "sgetopt.h"
#include "errtxt.h"
#include "idx.h"

#define FATAL "ezmlm-list: fatal: "

int flagnumber = 0;	/* default list subscribers, not number of */

void *psql = (void *) 0;

char strnum[FMT_ULONG];

void die_write()
{
  strerr_die3sys(111,FATAL,ERR_WRITE,"stdout: ");
}

void die_usage()
{
  strerr_die1x(100,"ezmlm-list: usage: ezmlm-list [-mMnNvV] dir");
}

static char outbuf[512];
static substdio ssout = SUBSTDIO_FDBUF(write,1,outbuf,sizeof(outbuf));

int subwrite(s,l)
char *s;
unsigned int l;
{
  return substdio_put(&ssout,s,l) | substdio_put(&ssout,"\n",1);
}

int dummywrite(s,l)
char *s;		/* ignored */
unsigned int l;
{
  return (int) l;
}

void main(argc,argv)
int argc;
char **argv;
{
  char *dir;
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
      case 'V': strerr_die2x(0,
	"ezmlm-list version: ezmlml-0.53+",EZIDX_VERSION);
      default:
	die_usage();
    }

  dir = argv[optind++];
  if (!dir) die_usage();

  if (chdir(dir) == -1)
    strerr_die4sys(111,FATAL,ERR_SWITCH,dir,": ");

  if (dir[0] != '/')
    strerr_die2x(100,FATAL,ERR_SLASH);

  if (flagnumber) {
    n = putsubs(dir,0L,52L,dummywrite,flagmysql,FATAL);
    if (substdio_put(&ssout,strnum,fmt_ulong(strnum,n)) == -1) die_write(FATAL);
    if (substdio_put(&ssout,"\n",1) == -1) die_write(FATAL);
  } else
    (void) putsubs(dir,0L,52L,subwrite,flagmysql,FATAL);
  if (substdio_flush(&ssout) == -1) die_write(FATAL);
  closesql();
  _exit(0);
}
