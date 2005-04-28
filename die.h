#ifndef DIE_H
#define DIE_H

extern const char FATAL[];
extern const char USAGE[];

extern void die_badaddr(void);
extern void die_badformat(void);
extern void die_dow(void);
extern void die_nomem(void);
extern void die_sender(void);
extern void die_usage(void);

#endif
