#ifndef ENV_H
#define ENV_H

extern int env_isinit;

extern int env_init(void);
extern int env_put(char *s);
extern int env_put2(const char *s,const char *t);
extern int env_unset(const char *s);
extern /*@null@*/char *env_get(const char *s);
extern void env_clear(void);
extern const char *env_findeq(const char *s);

extern char **environ;

#endif
