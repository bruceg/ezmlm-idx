/*$Id$*/

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "strerr.h"
#include "subscribe.h"
#include "sgetopt.h"
#include "str.h"
#include "stralloc.h"
#include "substdio.h"
#include "subfd.h"
#include "readwrite.h"
#include "getln.h"
#include "scan.h"
#include "errtxt.h"
#include "die.h"
#include "idx.h"
#include "config.h"
#include "auto_version.h"

stralloc line = {0};

void subunsub_main(int submode,
		   const char *version,
		   int argc,char **argv)
{
  const char *dir;
  const char *subdir;
  const char *addr;
  const char *comment;
  char *cp;
  char ch;
  int match;
  int flagmysql = 1;	/* use mysql if supported */
  int forcehash = -1;
  int flagname = 0;
  unsigned long u;
  int opt;
  char manual[8] = "+manual";

  manual[0] = submode ? '+' : '-';

  (void) umask(022);
  while ((opt = getopt(argc,argv,"h:HmMnNvV")) != opteof)
    switch(opt) {
      case 'h': (void) scan_ulong(optarg,&u); forcehash = 0; break;
      case 'H': forcehash = -1; break;
      case 'm': flagmysql = 1; break;
      case 'M': flagmysql = 0; break;
      case 'n': flagname = 1; break;
      case 'N': flagname = 0; break;
      case 'v':
      case 'V': strerr_die2x(0, version,auto_version);
      default:
	die_usage();
    }

  startup(dir = argv[optind++]);
  initsub(dir,flagmysql);
  /* If the second argument is present and does not contain a "@", treat
   * it as the subdirectory parameter and not an address to subscribe. */
  subdir = argv[optind];
  if (subdir != 0 && subdir[str_chr(subdir,'@')] == 0)
    ++optind;
  else
    subdir = 0;

  if (forcehash == 0) forcehash = (int) u;

  if (argv[optind]) {
    if (flagname) {
		/* allow repeats and last addr doesn't need comment */
      while ((addr = argv[optind++])) {
        (void) subscribe(subdir,addr,submode,argv[optind],manual,forcehash);
        if (!argv[optind++]) break;
      }
    } else {

      while ((addr = argv[optind++]))
        (void) subscribe(subdir,addr,submode,"",manual,forcehash);
    }
  } else {		/* stdin */
    for (;;) {
      if (getln(subfdin,&line,&match,'\n') == -1)
	strerr_die2sys(111,FATAL,ERR_READ_INPUT);
      if (!match) break;
      if (line.len == 1 || *line.s == '#') continue;
      line.s[line.len - 1] = '\0';
      comment = (char *) 0;
      if (flagname) {
	cp = line.s;
	while ((ch = *cp)) {
	  if (ch == '\\') {
            if (!*(++cp)) break;
	  } else if (ch == ' ' || ch == '\t' || ch == ',') break;
	  cp++;
        }
        if (ch) {
	  *cp = '\0';
	  comment = cp + 1;
        }
      }
      (void)subscribe(subdir,line.s,submode,comment,manual,forcehash);
    }
  }
  closesub();
  _exit(0);
}
