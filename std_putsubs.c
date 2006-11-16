/*$Id$*/

#include <unistd.h>
#include "getln.h"
#include "error.h"
#include "strerr.h"
#include "readwrite.h"
#include "str.h"
#include "fmt.h"
#include "stralloc.h"
#include "open.h"
#include "substdio.h"
#include "case.h"
#include "errtxt.h"
#include "sub_std.h"
#include "qmail.h"
#include "die.h"

static substdio ssin;
static char inbuf[512];
char strnum[FMT_ULONG];
static stralloc line = {0};
static stralloc fn = {0};

static void die_write(void)
{
  strerr_die3x(111,FATAL,ERR_WRITE,"stdout");
}

unsigned long std_putsubs(const char *dir,	/* database base dir */
			  const char *subdir,
			  unsigned long hash_lo,
			  unsigned long hash_hi,
			  int subwrite())	/* write function. */
/* Outputs all userhostesses in 'dbname' to stdout. If userhost is not null */
/* that userhost is excluded. 'dbname' is the base directory name. */
/* subwrite must be a*/
/* function returning >=0 on success, -1 on error, and taking arguments     */
/* (char* string, unsigned int length). It will be called once per address  */
/* and should take care of newline or whatever needed for the output form.  */

{

  unsigned int i;
  int fd;
  unsigned long no = 0L;
  int match;
  unsigned int hashpos;

    std_makepath(&fn,dir,subdir,"/subscribers/",'?');
    hashpos = fn.len - 2;
    if (hash_lo > 52) hash_lo = 52;
    if (hash_hi > 52) hash_hi = 52;
    if (hash_hi < hash_lo) hash_hi = hash_lo;

    for (i = hash_lo;i <= hash_hi;++i) {
      fn.s[hashpos] = 64 + i;	/* hash range 0-52 */
      fd = open_read(fn.s);
      if (fd == -1) {
        if (errno != error_noent)
	  strerr_die4sys(111,FATAL,ERR_READ,fn.s,": ");
      } else {
        substdio_fdbuf(&ssin,read,fd,inbuf,sizeof(inbuf));
        for (;;) {
          if (getln(&ssin,&line,&match,'\0') == -1)
            strerr_die4sys(111,FATAL,ERR_READ,fn.s,": ");
          if (!match)
            break;
          if (subwrite(line.s + 1,line.len - 2) == -1) die_write();
          no++;
        }
        close(fd);
      }
    }
    return no;
}
