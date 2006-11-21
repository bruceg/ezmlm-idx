/*$Id$*/
#ifndef SUB_STD_H
#define SUB_STD_H

#include "stralloc.h"

extern int std_subscribe(const char *subdir,
			 const char *username,
			 int flagadd,
			 const char *from,
			 const char *event,
			 int forcehash);

extern const char *std_issub(const char *subdir,
			     const char *username);

extern unsigned long std_putsubs(const char *subdir,
				 unsigned long hash_lo,
				 unsigned long hash_hi,
				 int subwrite());

/*		int subwrite(char *string, unsigned int length); */

extern void std_tagmsg(unsigned long msgnum,
		       const char *seed,
		       const char *action,
		       char *hashout);

extern const char *std_checktag(unsigned long msgnum,
				const char *action,
				const char *seed,
				const char *hash);

extern void std_searchlog(const char *subdir,
			  char *search,
			  int subwrite());

extern void std_makepath(stralloc *fn,
			 const char *subdir,
			 const char *append,
			 char ch);

#endif
