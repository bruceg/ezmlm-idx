/*$Id$*/
#ifndef SUBSCRIBE_H
#define SUBSCRIBE_H

#include "sqllib.h"

extern void initsub(const char *dir);

/* these are the subroutines used for interfacing with the subscriber and  */
/* moderator address databases. For the put/to address output routines     */
/* the 'username' if defined is omitted from the output. flagadd = 1 adds  */
/* a subscriber, flagadd = 0 removes the address. */

extern const char *checktag(const char *dir,
			    unsigned long msgnum,
			    unsigned long listno,
			    const char *action,
			    const char *seed,
			    const char *hash);

extern void closesub(void);

extern const char *issub(const char *dir,
			 const char *subdir,
			 const char *username);

extern unsigned long putsubs(const char *dir,
			     const char *subdir,
			     unsigned long hash_lo,
			     unsigned long hash_hi,
			     int subwrite(),
			     int flagsql);
/*		int subwrite(char *string, unsigned int length); */

extern const char *logmsg(const char *dir,
			  unsigned long msgnum,
			  unsigned long,
			  unsigned long subs,
			  int done);

extern void searchlog(const char *dir,
		      const char *subdir,
		      char *search,
		      int subwrite());

extern int subscribe(const char *dir,
		     const char *subdir,
		     const char *username,
		     int flagadd,
		     const char *from,
		     const char *event,
		     int flagmysql,
		     int forcehash);

extern void tagmsg(const char *dir,
		   unsigned long msgnum,
		   const char *seed,
		   const char *action,
		   char *hashout,
		   unsigned long bodysize,
		   unsigned long chunk);

#define SUB_PLUGIN_VERSION 1
struct sub_plugin
{
  int version;
  const char *(*checktag)(struct sqlinfo *info,
			  unsigned long msgnum,
			  unsigned long listno,
			  const char *hash);
  void (*close)(struct sqlinfo *info);
  const char *(*issub)(struct sqlinfo *info,
		       const char *username);
  const char *(*logmsg)(struct sqlinfo *info,
			unsigned long msgnum,
			unsigned long,
			unsigned long subs,
			int done);
  const char *(*open)(struct sqlinfo *info);
  unsigned long (*putsubs)(struct sqlinfo *info,
			   unsigned long hash_lo,
			   unsigned long hash_hi,
			   int subwrite());
/*		int subwrite(char *string, unsigned int length); */
  void (*searchlog)(struct sqlinfo *info,
		    char *search,
		    int subwrite());
  int (*subscribe)(struct sqlinfo *info,
		   const char *dir,
		   const char *subdir,
		   const char *username,
		   int flagadd,
		   const char *from,
		   const char *event,
		   int forcehash);
  void (*tagmsg)(struct sqlinfo *info,
		 const char *dir,
		 unsigned long msgnum,
		 char *hashout,
		 unsigned long bodysize,
		 unsigned long chunk);
};

#endif
