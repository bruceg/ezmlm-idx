#include "readwrite.h"
#include "substdio.h"
#include "subfd.h"

static char subfd_errbuf[256];
static substdio it = SUBSTDIO_FDBUF(write,2,subfd_errbuf,256);
substdio *subfderr = &it;
