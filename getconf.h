#ifndef GETCONF_H
#define GETCONF_H

extern int getconf(stralloc *sa,const char *fn,int flagrequired);
extern int getconf_line(stralloc *sa,const char *fn,int flagrequired);
extern int getconf_ulong(unsigned long *n,const char *fn,int flagrequired);
extern int getconf_ulong2(unsigned long *n0,unsigned long *n1,
			  const char *fn,int flagrequired);

#endif
