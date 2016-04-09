#ifndef QUOTE_H
#define QUOTE_H

extern int quote_need(const char *s,unsigned int n);
extern void quote(stralloc *saout,const stralloc *sain);
extern void quote2(stralloc *sa,const char *s);

#endif
