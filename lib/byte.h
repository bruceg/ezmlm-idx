/* Public domain, from daemontools-0.76. */

#ifndef BYTE_H
#define BYTE_H

#define _GNU_SOURCE
#include <string.h>

extern unsigned int byte_chr(const void *s,unsigned int n,int c);
extern unsigned int byte_rchr(const void *s,unsigned int n,int c);
#define byte_copy(TO,N,FROM) memcpy((TO),(FROM),(N))
#define byte_copyr(TO,N,FROM) memmove((TO),(FROM),(N))
#define byte_diff(S,N,T) memcmp((S),(T),(N))
#define byte_zero(S,N) memset((S),0,(N))
#define byte_equal(s,n,t) (!byte_diff((s),(n),(t)))

#endif
