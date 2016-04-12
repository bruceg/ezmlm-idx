/* Public domain, from daemontools-0.76. */

#ifndef STR_H
#define STR_H

#include "byte.h"

#define str_copy(TO,FROM) strcpy((TO),(FROM))
#define str_diff(A,B) strcmp((A),(B))
#define str_diffn(A,B,N) strncmp((A),(B),(N))
#define str_len(S) strlen(S)
extern unsigned int str_chr(const char *,int);
extern unsigned int str_rchr(const char *,int);
extern int str_start(const char *,const char *);

#define str_equal(s,t) (!str_diff((s),(t)))

#endif
