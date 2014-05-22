#ifndef CONFIG_H
#define CONFIG_H

#include "stralloc.h"

extern const char *listdir;
extern stralloc charset;
extern stralloc ezmlmrc;
extern stralloc key;
extern stralloc listid;
extern stralloc mainlocal;
extern stralloc outhost;
extern stralloc outlocal;
extern char flagcd;
extern int flag_isset(char flag);
extern int flag_isnameset(const char *name);

extern void startup(const char *dir);

#endif
