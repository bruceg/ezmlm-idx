#ifndef CONFIG_H
#define CONFIG_H

#include "stralloc.h"

extern stralloc key;

extern void startup(const char *dir);
extern void load_config(void);

#endif
