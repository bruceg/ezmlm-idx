/*$Id$*/
#ifndef SUBSCRIBE_H
#define SUBSCRIBE_H

#include "stralloc.h"

/* these are the subroutines used for interfacing with the subscriber and  */
/* moderator address databases. For the put/to address output routines     */
/* the 'username' if defined is omitted from the output. flagadd = 1 adds  */
/* a subscriber, flagadd = 0 removes the address. To use e.g. a SQL data-  */
/* base for addresses, just replace these routines and rebuild ezmlm.      */

#ifdef WITH_PROTO

extern int subscribe(char *dir,char *username,int flagadd,char *from,
	char *event, int flagmysql, int forcehash,
	char *table_override, char *FATAL);

extern char *issub(char *dir,char *username, char *table_override, char *FATAL);

extern unsigned long putsubs(char *dir,
	unsigned long hash_lo, unsigned long hash_hi,
	int subwrite(), int flagsql, char *fatal);

/*		int subwrite(char *string, unsigned int length); */

extern void tagmsg(char *dir, unsigned long msgnum,
	char *seed, char *action, char *hashout,
	unsigned long bodysize, unsigned long chunk, char *fatal);

extern char *logmsg(char *dir, unsigned long msgnum, unsigned long,
	unsigned long subs, int done);

extern char *checktag(char *dir, unsigned long msgnum, unsigned long listno,
	char *action, char *seed, char *hash);

extern void searchlog(char *dir, char *search, int subwrite(), char *fatal);

extern char *opensql(char *dir, char **table);

extern void closesql();

#else

extern int subscribe();
extern char *issub();
extern unsigned long putsubs();
extern void tagmsg();
extern char *logmsg();
char *checktag();
extern int subreceipt();
extern char *getlistno();
extern char *opensql();
extern void closesql();
extern void searchlog();

#endif
extern void *psql;		/* contains SQL handle */
#endif
