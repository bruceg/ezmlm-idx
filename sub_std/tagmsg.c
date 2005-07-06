/*$Id$*/

#include "subscribe.h"

void tagmsg(const char *dir,		/* db base dir */
	    unsigned long msgnum,	/* number of this message */
	    const char *seed,		/* seed. NULL ok, but less entropy */
	    const char *action,	/* to make it certain the cookie differs from*/
				/* one used for a digest */
	    char *hashout,		/* calculated hash goes here */
	    unsigned long bodysize,
	    unsigned long chunk)
{
  std_tagmsg(dir,msgnum,seed,action,hashout);
  (void)bodysize;
  (void)chunk;
}
