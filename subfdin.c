#include "readwrite.h"
#include "substdio.h"
#include "subfd.h"

static char inbuf[1024];
static substdio it = SUBSTDIO_FDBUF(read,0,inbuf,sizeof inbuf);
substdio *const subfdin = &it;
