/* Public domain, from daemontools-0.76. */

#ifndef WAIT_H
#define WAIT_H

extern int wait_pid(int *wstat,int pid);

#define wait_crashed(w) ((w) & 127)
#define wait_exitcode(w) ((w) >> 8)
#define wait_stopsig(w) ((w) >> 8)
#define wait_stopped(w) (((w) & 127) == 127)

#endif
