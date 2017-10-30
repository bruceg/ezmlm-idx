/* Public domain, from daemontools-0.76. */

#ifndef STRALLOC_H
#define STRALLOC_H

#include "gen_alloc.h"

GEN_ALLOC_typedef(stralloc,char,s,len,a)

extern void stralloc_ready(stralloc *,unsigned int);
extern void stralloc_readyplus(stralloc *,unsigned int);
extern void stralloc_copy(stralloc *,const stralloc *);
extern void stralloc_cat(stralloc *,const stralloc *);
extern void stralloc_copys(stralloc *,const char *);
extern void stralloc_cats(stralloc *,const char *);
extern void stralloc_copyb(stralloc *,const char *,unsigned int);
extern void stralloc_catb(stralloc *,const char *,unsigned int);
extern void stralloc_append(stralloc *,char);
extern int stralloc_starts(stralloc *,const char *);

#define stralloc_0(sa) stralloc_append(sa,'\0')

extern void stralloc_catulong0(stralloc *,unsigned long,unsigned int);
extern void stralloc_catlong0(stralloc *,long,unsigned int);

#define stralloc_catlong(sa,l) (stralloc_catlong0((sa),(l),0))
#define stralloc_catuint0(sa,i,n) (stralloc_catulong0((sa),(i),(n)))
#define stralloc_catint0(sa,i,n) (stralloc_catlong0((sa),(i),(n)))
#define stralloc_catint(sa,i) (stralloc_catlong0((sa),(i),0))

#endif