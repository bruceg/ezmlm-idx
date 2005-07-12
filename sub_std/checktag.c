/*$Id$*/

#include "subscribe.h"

const char *checktag(const char *dir,		/* the db base dir */
		     unsigned long num,		/* message number */
		     unsigned long listno,	/* bottom of range => slave */
		     const char *action,
		     const char *seed,		/* cookie base */
		     const char *hash)		/* cookie */
{
  return std_checktag(num,action,seed,hash);
  (void)dir;
  (void)listno;
}
