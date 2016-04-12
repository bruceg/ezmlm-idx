/* Public domain, from ezmlm-0.53. */

#ifndef SGETOPT_H
#define SGETOPT_H

#ifndef SGETOPTNOSHORT
#define getopt sgetoptmine
#define optarg subgetoptarg
#define optind subgetoptind
#define optpos subgetoptpos
#define opterr sgetopterr
#define optproblem subgetoptproblem
#define optprogname sgetoptprogname
#define opteof subgetoptdone
#endif

#include "subgetopt.h"

extern int sgetoptmine(int,char *const *,const char *);
extern int sgetopterr;
extern const char *sgetoptprogname;

#endif
