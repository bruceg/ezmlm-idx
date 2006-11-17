#include "readwrite.h"
#include "substdio.h"
#include "subfd.h"

static char errbuf[256];
static substdio it = SUBSTDIO_FDBUF(write,2,errbuf,sizeof errbuf);
substdio *const subfderr = &it;
