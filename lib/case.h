/* Public domain, from djbdns-1.05. */
/* As per http://cr.yp.to/djbdns/res-disaster.html */

#ifndef CASE_H
#define CASE_H

extern void case_lowers(char *);
extern void case_lowerb(char *,unsigned int);
extern int case_diffs(const char *,const char *);
extern int case_diffb(const char *,unsigned int,const char *);
extern int case_starts(const char *,const char *);
extern int case_startb(const char *,unsigned int,const char *);

#define case_equals(s,t) (!case_diffs((s),(t)))

#endif
