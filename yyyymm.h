#ifndef YYYYMM_H
#define YYYYMM_H

#ifdef WITH_PROTO
#include "stralloc.h"

extern unsigned int date2yyyymm(char *);
extern int dateline(stralloc *, unsigned long);
#else
extern unsigned int date2yyyymm();
extern int dateline();
#endif

#endif

