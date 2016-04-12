/* Public domain, from daemontools-0.76. */

#include <sys/types.h>
#include <sys/wait.h>

main()
{
  waitpid(0,0,0);
}
