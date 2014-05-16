#ifndef CGI_H
#define CGI_H

#include "sys/hasattribute.h"

extern void cgierr(const char *,const char *,const char *) __attribute((noreturn));

#endif
