#ifndef QUOTE_H
#define QUOTE_H

extern int quote_need(const char *s,unsigned int n);
extern int quote(stralloc *saout,const stralloc *sain);
extern int quote2(stralloc *sa,const char *s);

#endif
