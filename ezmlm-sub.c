/*$Id$*/

#include "strerr.h"
#include "errtxt.h"

const char FATAL[] = "ezmlm-sub: fatal: ";

void die_nomem(void) { strerr_die2x(111,FATAL,ERR_NOMEM); }

void die_usage(void)
{
  strerr_die1x(100,
	       "ezmlm-sub: usage: ezmlm-sub [-mMvV] [-h hash] [-n] dir "
	       "[box@domain [name]] ...");
}

extern void subunsub_main(int submode,
			  const char *version,
			  int argc,char **argv);

void main(int argc,char **argv)
{
  subunsub_main(1,"ezmlm-sub version: ",argc,argv);
}
