#include <signal.h>
#include "sig.h"
#include "sys/hassgact.h"

void sig_catch(int sig,void (*f)(int))
{
#ifdef HASSIGACTION
  struct sigaction sa;
  sa.sa_handler = f;
  sa.sa_flags = 0;
  sigemptyset(&sa.sa_mask);
  sigaction(sig,&sa,(struct sigaction *) 0);
#else
  signal(sig,f); /* won't work under System V, even nowadays---dorks */
#endif
}
