/*$Id$*/

#include "strerr.h"
#include "errtxt.h"

const char FATAL[] = "ezmlm-unsub: fatal: ";

void die_nomem(void) { strerr_die2x(111,FATAL,ERR_NOMEM); }

void die_usage(void)
{
  strerr_die1x(100,
	       "ezmlm-unsub: usage: ezmlm-unsub [-h hash] [-HmMnNvV] dir "
	       "[box@domain ...]");
}

extern void subunsub_main(int submode,
			  const char *version,
			  int argc,char **argv);

void main(int argc,char **argv)
{
  subunsub_main(0,"ezmlm-unsub version: ",argc,argv);
}
