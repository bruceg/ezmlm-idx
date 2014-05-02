#ifndef DMARC_H
#define DMARC_H

#include "stralloc.h"

extern int dmarc_fetch(stralloc *out,const char *domain);
extern int dmarc_get(const stralloc *rec,const char *key,stralloc *out);
extern int dmarc_p_reject(const char *domain);

#endif
