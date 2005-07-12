/*$Id$*/

#include "subscribe.h"

unsigned long putsubs(const char *dir,		/* database base dir */
		      const char *subdir,
		      unsigned long hash_lo,
		      unsigned long hash_hi,
		      int subwrite(),		/* write function. */
		      int flagsql)
{
  return std_putsubs(dir,subdir,hash_lo,hash_hi,subwrite);
  (void)flagsql;
}
