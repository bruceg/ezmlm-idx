#ifndef ENV_H
#define ENV_H

extern int env_isinit;

extern void env_put(char *s);
extern void env_put2(const char *s,const char *t);
extern void env_unset(const char *s);
extern /*@null@*/char *env_get(const char *s);
extern void env_clear(void);
extern const char *env_findeq(const char *s);

extern char **environ;

#endif
