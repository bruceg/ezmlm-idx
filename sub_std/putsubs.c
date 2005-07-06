/*$Id$*/

#include "subscribe.h"

unsigned long putsubs(const char *dbname,	/* database base dir */
		      unsigned long hash_lo,
		      unsigned long hash_hi,
		      int subwrite(),		/* write function. */
		      int flagsql)
{
  return std_putsubs(dbname,hash_lo,hash_hi,subwrite);
  (void)flagsql;
}
