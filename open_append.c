/* Public domain, from daemontools-0.76. */

#include <sys/types.h>
#include <fcntl.h>
#include "open.h"

int open_append(const char *fn)
{ return open(fn,O_WRONLY | O_NDELAY | O_APPEND | O_CREAT,0600); }
