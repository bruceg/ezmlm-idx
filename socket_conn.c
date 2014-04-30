/* Public domain, from djbdns-1.05. */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include "byte.h"
#include "socket.h"

int socket_connect4(int s,const char ip[4],uint16 port)
{
  struct sockaddr_in sa;

  byte_zero((void*)&sa,sizeof sa);
  sa.sin_family = AF_INET;
  uint16_pack_big((char *) &sa.sin_port,port);
  byte_copy((char *) &sa.sin_addr,4,ip);

  return connect(s,(struct sockaddr *) &sa,sizeof sa);
}

int socket_connected(int s)
{
  struct sockaddr_in sa;
  socklen_t dummy;
  char ch;

  dummy = sizeof sa;
  if (getpeername(s,(struct sockaddr *) &sa,&dummy) == -1) {
    read(s,&ch,1); /* sets errno */
    return 0;
  }
  return 1;
}
