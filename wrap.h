#ifndef WRAP_H
#define WRAP_H

extern void wrap_execv(const char **argv, const char *FATAL);
extern void wrap_execvp(const char **argv, const char *FATAL);
extern void wrap_exitcode(int pid, const char *FATAL);
extern int wrap_waitpid(int pid, const char *FATAL);

#endif
