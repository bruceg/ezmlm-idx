#include "readwrite.h"
#include "substdio.h"
#include "subfd.h"

static char outbuf[1024];
static substdio it = SUBSTDIO_FDBUF(write,1,outbuf,sizeof outbuf);
substdio *const subfdout = &it;
