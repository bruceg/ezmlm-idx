/*$Id$*/

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "strerr.h"
#include "subscribe.h"
#include "sgetopt.h"
#include "stralloc.h"
#include "substdio.h"
#include "readwrite.h"
#include "getln.h"
#include "scan.h"
#include "errtxt.h"
#include "idx.h"
#include "auto_version.h"

char inbuf[512];
substdio ssin = SUBSTDIO_FDBUF(read,0,inbuf,sizeof(inbuf));

stralloc line = {0};

extern void die_usage(void);

void subunsub_main(int submode,
		   const char *version,
		   int argc,char **argv)
{
  char *dir;
  char *addr;
  char *comment;
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

  dir = argv[optind++];
  if (!dir) die_usage();

  if (dir[0] != '/')
    strerr_die2x(100,FATAL,ERR_SLASH);

  if (chdir(dir) == -1)
    strerr_die4sys(111,FATAL,ERR_SWITCH,dir,": ");

  if (forcehash == 0) forcehash = (int) u;

  if (argv[optind]) {
    if (flagname) {
		/* allow repeats and last addr doesn't need comment */
      while ((addr = argv[optind++])) {
        (void) subscribe(dir,addr,submode,argv[optind],manual,
		flagmysql,forcehash,(char *) 0);
        if (!argv[optind++]) break;
      }
    } else {

      while ((addr = argv[optind++]))
        (void) subscribe(dir,addr,submode,"",manual,flagmysql,
		forcehash,(char *) 0);
    }
  } else {		/* stdin */
    for (;;) {
      if (getln(&ssin,&line,&match,'\n') == -1)
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
      (void) subscribe(dir,line.s,submode,comment,manual,flagmysql,
		forcehash,(char *) 0);
    }
  }
  closesql();
  _exit(0);
}
