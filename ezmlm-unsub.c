#include "strerr.h"
#include "subscribe.h"
#include "sgetopt.h"
#include "substdio.h"
#include "readwrite.h"
#include "getln.h"
#include "scan.h"
#include "errtxt.h"
#include "idx.h"
#include "auto_version.h"

#define FATAL "ezmlm-unsub: fatal: "

void *psql = (void *) 0;

char inbuf[512];
substdio ssin = SUBSTDIO_FDBUF(read,0,inbuf,sizeof(inbuf));

stralloc line = {0};

void die_usage()
{
  strerr_die1x(100,
   "ezmlm-unsub: usage: ezmlm-unsub [-h hash] [-HmMnNvV] dir box@domain ...");
}

void main(argc,argv)
int argc;
char **argv;
{
  char *dir;
  char *addr;
  char *cp;
  char ch;
  int match;
  int flagmysql = 1;		/* if there is mysql support, use it! */
  int forcehash = -1;
  int flagname = 0;
  unsigned long u;
  int opt;

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
      case 'V': strerr_die2x(0, "ezmlm-unsub version: ",auto_version);
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
    while ((addr = argv[optind++]))
      (void) subscribe(dir,addr,0,"","-manual",flagmysql,
		forcehash,(char *) 0,FATAL);
  } else {
    for (;;) {
      if (getln(&ssin,&line,&match,'\n') == -1)
	strerr_die2sys(111,FATAL,ERR_READ_INPUT);
      if (!match) break;
      if (line.len == 1 || *line.s == '#') continue;
      line.s[line.len - 1] = '\0';
      if (flagname) {
	cp = line.s;
	while ((ch = *cp)) {
	  if (ch == '\\') {
            if (!*(++cp)) break;
	  } else if (ch == ' ' || ch == '\t' || ch == ',') break;
	  cp++;
	}
	if (ch)
	  *cp = '\0';
      }
      (void) subscribe(dir,line.s,0,"","-manual",flagmysql,
		forcehash,(char *) 0,FATAL);
    }
  }
  closesql();
  _exit(0);
}
