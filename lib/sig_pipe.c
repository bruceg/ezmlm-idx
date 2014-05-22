#include <signal.h>
#include "sig.h"

void sig_pipeignore(void) { sig_catch(SIGPIPE,SIG_IGN); }
void sig_pipedefault(void) { sig_catch(SIGPIPE,SIG_DFL); }
