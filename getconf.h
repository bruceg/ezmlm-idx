#ifndef GETCONF_H
#define GETCONF_H

extern int getconf(stralloc *sa,const char *fn,int flagrequired,
		   const char *dir);
extern int getconf_line(stralloc *sa,const char *fn,int flagrequired,
			const char *dir);
extern int getconf_ulong(unsigned long *n,const char *fn,int flagrequired,
			 const char *dir);

#endif
