#ifndef COPY_H
#define COPY_H

extern void copy(struct qmail *qq,const char *fn,char q);
extern void set_cpoutlocal(const stralloc *);
extern void set_cpouthost(const stralloc *);
extern void set_cptarget(const char *);
extern void set_cpconfirm(const char *);
extern void set_cpnum(const char *);
extern void set_cpverptarget(const char *);

#endif
