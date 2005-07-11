#ifndef DIE_H
#define DIE_H

#include "hasattribute.h"

extern const char FATAL[];
extern const char USAGE[];

extern void die_badaddr(void) __attribute__((noreturn));
extern void die_badformat(void) __attribute__((noreturn));
extern void die_dow(void) __attribute__((noreturn));
extern void die_nomem(void) __attribute__((noreturn));
extern void die_sender(void) __attribute__((noreturn));
extern void die_usage(void) __attribute__((noreturn));

#endif
