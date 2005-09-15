#ifndef CONFIG_H
#define CONFIG_H

#include "stralloc.h"

extern stralloc charset;
extern stralloc key;
extern stralloc listid;
extern stralloc mailinglist;
extern stralloc outhost;
extern stralloc outlocal;
extern char flagcd;

extern void startup(const char *dir);
extern void load_config(const char *dir);

#endif
