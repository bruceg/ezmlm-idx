#ifndef WRAP_H
#define WRAP_H

struct stralloc;
struct stat;

extern void wrap_execbin(const char *program,
	                 struct stralloc *opts,
	                 const char *dir);
extern void wrap_execsh(const char *command);
extern void wrap_execv(const char **argv);
extern void wrap_execvp(const char **argv);
extern void wrap_exitcode(int pid);
extern void wrap_chdir(const char *dir);
extern int wrap_fork(void);
extern void wrap_rename(const char *oldpath,const char *newpath);
extern int wrap_stat(const char *fn,struct stat *st);
extern int wrap_waitpid(int pid);

#endif
