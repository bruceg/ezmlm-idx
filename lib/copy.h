#ifndef COPY_H
#define COPY_H

#include "cookie.h"
#include "datetime.h"

struct qmail;

extern void codeput(struct qmail *qq,const char *l,unsigned int n,char code);
extern void codeputs(struct qmail *qq,const char *l,char code);
extern void copy_xlate(stralloc *out,const stralloc *in,
		       const char *params[10],char q);
extern void copy(struct qmail *qq,const char *fn,char q);
extern void set_cpaction(const char *ac);
extern void set_cptarget(const char *);
extern void set_cpconfirm(const char *,unsigned int);
extern void set_cphash(const char h[COOKIE]);
extern void set_cpnum(const char *);
extern void set_cpverptarget(const char *);
extern void set_cpwhen(datetime_sec t);

#endif
