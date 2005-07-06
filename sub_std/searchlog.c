/*$Id$*/

#include "subscribe.h"

void searchlog(const char *dir,		/* work directory */
	       char *search,		/* search string */
	       int subwrite())		/* output fxn */
{
  std_searchlog(dir,search,subwrite);
}
