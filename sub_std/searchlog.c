/*$Id$*/

#include "subscribe.h"

void searchlog(const char *dir,		/* work directory */
	       const char *subdir,
	       char *search,		/* search string */
	       int subwrite())		/* output fxn */
{
  std_searchlog(dir,subdir,search,subwrite);
}
