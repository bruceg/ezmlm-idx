#ifndef QMAIL_H
#define QMAIL_H

#include "substdio.h"
#include "stralloc.h"

struct qmail {
  int flagerr;
  unsigned long pid;
  unsigned long msgbytes;
  int fdm;
  int fde;
  substdio ss;
  char buf[1024];
} ;

#ifdef WITH_PROTO

extern int qmail_open(struct qmail *, stralloc *);
extern void qmail_put(struct qmail *, char *, int);
extern void qmail_puts(struct qmail *, char *);
extern void qmail_from(struct qmail *, char *);
extern void qmail_to(struct qmail *, char *);
extern void qmail_fail(struct qmail *);
extern char *qmail_close(struct qmail *);
extern unsigned long qmail_qp(struct qmail *);

#else

extern int qmail_open();
extern void qmail_put();
extern void qmail_puts();
extern void qmail_from();
extern void qmail_to();
extern void qmail_fail();
extern char *qmail_close();
extern unsigned long qmail_qp();
#endif

#endif
