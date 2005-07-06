/*$Id$*/
#ifndef SUBSCRIBE_H
#define SUBSCRIBE_H

#include "stralloc.h"

/* these are the subroutines used for interfacing with the subscriber and  */
/* moderator address databases. For the put/to address output routines     */
/* the 'username' if defined is omitted from the output. flagadd = 1 adds  */
/* a subscriber, flagadd = 0 removes the address. To use e.g. a SQL data-  */
/* base for addresses, just replace these routines and rebuild ezmlm.      */

extern int subscribe(const char *dir,
		     const char *username,
		     int flagadd,
		     const char *from,
		     const char *event,
		     int flagmysql,
		     int forcehash);
extern int std_subscribe(const char *dir,
			 const char *username,
			 int flagadd,
			 const char *from,
			 const char *event,
			 int forcehash);

extern const char *issub(const char *dir,
			 const char *username);
extern const char *std_issub(const char *dir,
			     const char *username);

extern unsigned long putsubs(const char *dir,
			     unsigned long hash_lo,
			     unsigned long hash_hi,
			     int subwrite(),
			     int flagsql);
extern unsigned long std_putsubs(const char *dir,
				 unsigned long hash_lo,
				 unsigned long hash_hi,
				 int subwrite());

/*		int subwrite(char *string, unsigned int length); */

extern void tagmsg(const char *dir,
		   unsigned long msgnum,
		   const char *seed,
		   const char *action,
		   char *hashout,
		   unsigned long bodysize,
		   unsigned long chunk);
extern void std_tagmsg(const char *dir,
		       unsigned long msgnum,
		       const char *seed,
		       const char *action,
		       char *hashout);

extern const char *logmsg(const char *dir,
			  unsigned long msgnum,
			  unsigned long,
			  unsigned long subs,
			  int done);

extern const char *checktag(const char *dir,
			    unsigned long msgnum,
			    unsigned long listno,
			    const char *action,
			    const char *seed,
			    const char *hash);
extern const char *std_checktag(const char *dir,
				unsigned long msgnum,
				const char *action,
				const char *seed,
				const char *hash);

extern void searchlog(const char *dir,
		      char *search,
		      int subwrite());
extern void std_searchlog(const char *dir,
			  char *search,
			  int subwrite());

extern const char *opensub(const char *dir,const char **table);

extern void closesub(void);

struct sqlinfo
{
  const char *host;
  const char *port;
  const char *db;
  const char *user;
  const char *pw;
};

extern const char *parsesql(const char *dbname,
			    const char **table,
			    struct sqlinfo *info);

#endif
