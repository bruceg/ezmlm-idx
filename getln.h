#ifndef GETLN_H
#define GETLN_H

struct substdio;
struct stralloc;

extern int getln(struct substdio *ss,struct stralloc *sa,
		 int *match,int sep);
extern int getln2(struct substdio *ss,struct stralloc *sa,
		  /*@out@*/char **cont,/*@out@*/unsigned int *clen,int sep);

#endif
