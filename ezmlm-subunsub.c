#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "strerr.h"
#include "subdb.h"
#include "getconfopt.h"
#include "str.h"
#include "stralloc.h"
#include "substdio.h"
#include "subfd.h"
#include "readwrite.h"
#include "getln.h"
#include "scan.h"
#include "messages.h"
#include "die.h"
#include "idx.h"
#include "config.h"
#include "auto_version.h"

static stralloc event;

static unsigned long hash;
static const char *flagsubdb = 0;
static int forcehash = -1;
static int flagname = 0;
static const char *flagtag = "manual";

static struct option options[] = {
  OPT_ULONG(hash,'h',0),
  OPT_FLAG(forcehash,'H',-1,0),
  OPT_CSTR_FLAG(flagsubdb,'m',0,0),
  OPT_CSTR_FLAG(flagsubdb,'M',"std",0),
  OPT_CSTR(flagsubdb,'S',0),
  OPT_FLAG(flagname,'n',1,0),
  OPT_FLAG(flagname,'N',0,0),
  OPT_CSTR(flagtag,'t',0),
  OPT_END
};

void subunsub_main(int submode,
		   int argc,char **argv)
{
  const char *subdir;
  const char *addr;
  const char *comment;
  char *cp;
  char ch;
  int match;
  int i;
  stralloc line = {0};

  (void) umask(022);
  i = getconfopt(argc,argv,options,1,0);
  initsub(flagsubdb);
  /* If the second argument is present and does not contain a "@", treat
   * it as the subdirectory parameter and not an address to subscribe. */
  subdir = argv[i];
  if (subdir != 0 && subdir[str_chr(subdir,'@')] == 0)
    ++i;
  else
    subdir = 0;

  if (hash != 0)
    forcehash = (int) hash;

  if (!stralloc_copys(&event,submode ? "+" : "-")) die_nomem();
  if (!stralloc_cats(&event,flagtag)) die_nomem();
  if (!stralloc_0(&event)) die_nomem();

  if (argv[i]) {
    if (flagname) {
		/* allow repeats and last addr doesn't need comment */
      while ((addr = argv[i++])) {
        (void) subscribe(subdir,addr,submode,argv[i],event.s,forcehash);
        if (!argv[i++]) break;
      }
    } else {

      while ((addr = argv[i++]))
        (void) subscribe(subdir,addr,submode,"",event.s,forcehash);
    }
  } else {		/* stdin */
    for (;;) {
      if (getln(subfdin,&line,&match,'\n') == -1)
	strerr_die2sys(111,FATAL,MSG(ERR_READ_INPUT));
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
      (void)subscribe(subdir,line.s,submode,comment,event.s,forcehash);
    }
  }
  closesub();
  _exit(0);
}
