#ifndef SLURP_H
#define SLURP_H

struct stralloc;
extern int slurp(const char *fn,struct stralloc *sa,int bufsize);

#endif
