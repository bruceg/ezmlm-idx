#ifndef COPY_H
#define COPY_H

struct qmail;

extern void copy_xlate(stralloc *out,const stralloc *in,char q);
extern void copy(struct qmail *qq,const char *fn,char q);
extern void set_cptarget(const char *);
extern void set_cpconfirm(const char *,unsigned int);
extern void set_cpnum(const char *);
extern void set_cpverptarget(const char *);

#endif
