#ifndef WRAP_H
#define WRAP_H

struct stralloc;
struct stat;

extern void wrap_execbin(const char *program,
	                 struct stralloc *opts,
	                 const char *dir,
	                 const char *FATAL);
extern void wrap_execsh(const char *command, const char *FATAL);
extern void wrap_execv(const char **argv, const char *FATAL);
extern void wrap_execvp(const char **argv, const char *FATAL);
extern void wrap_exitcode(int pid, const char *FATAL);
extern int wrap_fork(const char *FATAL);
extern int wrap_waitpid(int pid, const char *FATAL);
extern int wrap_stat(const char *fn,struct stat *st,const char *FATAL);

#endif
