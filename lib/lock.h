/* Public domain, from daemontools-0.76. */

#ifndef LOCK_H
#define LOCK_H

extern int lock_ex(int);
extern int lockfile(const char *path);

#endif
