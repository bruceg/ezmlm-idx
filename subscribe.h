/*$Id$*/
#ifndef SUBSCRIBE_H
#define SUBSCRIBE_H

extern void initsub(const char *dir);

/* these are the subroutines used for interfacing with the subscriber and  */
/* moderator address databases. For the put/to address output routines     */
/* the 'username' if defined is omitted from the output. flagadd = 1 adds  */
/* a subscriber, flagadd = 0 removes the address. */

typedef void (*closesub_fn)(void);

typedef int (*subscribe_fn)(const char *dir,
			    const char *subdir,
			    const char *username,
			    int flagadd,
			    const char *from,
			    const char *event,
			    int flagmysql,
			    int forcehash);

typedef const char *(*issub_fn)(const char *dir,
				const char *subdir,
				const char *username);

typedef unsigned long (*putsubs_fn)(const char *dir,
				    const char *subdir,
				    unsigned long hash_lo,
				    unsigned long hash_hi,
				    int subwrite(),
				    int flagsql);

/*		int subwrite(char *string, unsigned int length); */

typedef void (*tagmsg_fn)(const char *dir,
			  unsigned long msgnum,
			  const char *seed,
			  const char *action,
			  char *hashout,
			  unsigned long bodysize,
			  unsigned long chunk);

typedef const char *(*logmsg_fn)(const char *dir,
				 unsigned long msgnum,
				 unsigned long,
				 unsigned long subs,
				 int done);

typedef const char *(*checktag_fn)(const char *dir,
				   unsigned long msgnum,
				   unsigned long listno,
				   const char *action,
				   const char *seed,
				   const char *hash);

typedef void (*searchlog_fn)(const char *dir,
			     const char *subdir,
			     char *search,
			     int subwrite());

extern checktag_fn checktag;
extern closesub_fn closesub;
extern issub_fn issub;
extern logmsg_fn logmsg;
extern putsubs_fn putsubs;
extern tagmsg_fn tagmsg;
extern searchlog_fn searchlog;
extern subscribe_fn subscribe;

#endif
