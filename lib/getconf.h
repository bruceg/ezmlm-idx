#ifndef GETCONF_H
#define GETCONF_H

struct stralloc;
extern int getconf(struct stralloc *sa,const char *fn,int flagrequired);
extern int getconf_isset(const char *fn);
extern int getconf_line(struct stralloc *sa,const char *fn,int flagrequired);
extern int getconf_ulong(unsigned long *n,const char *fn,int flagrequired);
extern int getconf_ulong2(unsigned long *n0,unsigned long *n1,
			  const char *fn,int flagrequired);

#endif
