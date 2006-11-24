#ifndef CONFIG_H
#define CONFIG_H

#include "stralloc.h"

#define NO_FLAGS ('z' - 'a' + 1)

extern const char *listdir;
extern stralloc charset;
extern stralloc ezmlmrc;
extern stralloc key;
extern stralloc listid;
extern stralloc local;
extern stralloc mailinglist;
extern stralloc outhost;
extern stralloc outlocal;
extern char flagcd;
extern int flags[NO_FLAGS];

extern void startup(const char *dir);
extern void load_config(const char *dir);

#endif
