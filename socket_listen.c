/* Public domain, from djbdns-1.05. */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "socket.h"

int socket_listen(int s,int backlog)
{
  return listen(s,backlog);
}
