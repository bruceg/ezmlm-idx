/*$Id$*/
#ifndef SUBSCRIBE_H
#define SUBSCRIBE_H

#include "stralloc.h"

struct subdbinfo
{
  const char *plugin;
  const char *host;
  unsigned long port;
  const char *db;
  const char *user;
  const char *pw;
  const char *base_table;
  void *conn;
};

extern void initsub(const char *subdbline);

extern const char *mktab(void);

/* these are the subroutines used for interfacing with the subscriber and  */
/* moderator address databases. For the put/to address output routines     */
/* the 'username' if defined is omitted from the output. flagadd = 1 adds  */
/* a subscriber, flagadd = 0 removes the address. */

extern const char *checktag(unsigned long msgnum,
			    unsigned long listno,
			    const char *action,
			    const char *seed,
			    const char *hash);

extern void closesub(void);

/* Returns 1 if userhost is in the subscriber database and sets recorded
 * (if it is not NULL) to the stored address, 0 otherwise. */
extern int issub(const char *subdir,
		 const char *userhost,
		 stralloc *recorded);

extern unsigned long putsubs(const char *subdir,
			     unsigned long hash_lo,
			     unsigned long hash_hi,
			     int subwrite());
/*		int subwrite(char *string, unsigned int length); */

extern const char *logmsg(unsigned long msgnum,
			  unsigned long,
			  unsigned long subs,
			  int done);

extern void searchlog(const char *subdir,
		      char *search,
		      int subwrite());

extern int subscribe(const char *subdir,
		     const char *username,
		     int flagadd,
		     const char *from,
		     const char *event,
		     int forcehash);

extern void tagmsg(unsigned long msgnum,
		   const char *seed,
		   const char *action,
		   char *hashout,
		   unsigned long bodysize,
		   unsigned long chunk);

#define SUB_PLUGIN_VERSION 2
struct sub_plugin
{
  int version;
  const char *(*checktag)(struct subdbinfo *info,
			  unsigned long msgnum,
			  unsigned long listno,
			  const char *action,
			  const char *seed,
			  const char *hash);
  void (*close)(struct subdbinfo *info);
  int (*issub)(struct subdbinfo *info,
	       const char *table,
	       const char *username,
	       stralloc *recorded);
  const char *(*logmsg)(struct subdbinfo *info,
			unsigned long msgnum,
			unsigned long listno,
			unsigned long subs,
			int done);
  const char *(*mktab)(struct subdbinfo *info);
  const char *(*open)(struct subdbinfo *info);
  unsigned long (*putsubs)(struct subdbinfo *info,
			   const char *table,
			   unsigned long hash_lo,
			   unsigned long hash_hi,
			   int subwrite());
  void (*searchlog)(struct subdbinfo *info,
		    const char *table,
		    char *search,
		    int subwrite());
  int (*subscribe)(struct subdbinfo *info,
		   const char *table,
		   const char *username,
		   int flagadd,
		   const char *from,
		   const char *event,
		   int forcehash);
  void (*tagmsg)(struct subdbinfo *info,
		 unsigned long msgnum,
		 const char *hashout,
		 unsigned long bodysize,
		 unsigned long chunk);
};

#endif
