#ifndef ALT_H
#define ALT_H

struct stralloc;

const char *altpath(struct stralloc *s,const char *name);
int alt_open_read(const char *fn);

#endif
