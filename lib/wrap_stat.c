/*$Id$*/

#include <sys/types.h>
#include <sys/stat.h>
#include "error.h"
#include "errtxt.h"
#include "strerr.h"
#include "wrap.h"
#include "die.h"
#include "idx.h"

int wrap_stat(const char *fn,struct stat *st)
{
  int r;
  if ((r = stat(fn,st)) == -1
      && errno != error_noent)
    strerr_die4sys(111,FATAL,ERR_STAT,fn,": ");
  return r;
}

    
