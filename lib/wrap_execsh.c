#include "wrap.h"

void wrap_execsh(const char *command)
{
  const char *args[4];
  args[0] = "/bin/sh";
  args[1] = "-c";
  args[2] = command;
  args[3] = 0;
  wrap_execv(args);
}
