/*$Id$*/

#include "qmail.h"
#include "substdio.h"

int qmail_copy(struct qmail *qq,substdio *ss,long lines)
{
  int n;
  int i;
  char *x;
  int in_header;
  int was_lf;

  for (in_header = 1, was_lf = 0;;) {
    n = substdio_feed(ss);
    if (n < 0) return -2;
    x = substdio_PEEK(ss);
    if (lines >= 0) {
      i = 0;
      if (in_header) {
	for (; i < n; ++i) {
	  if (x[i] == '\n') {
	    if (was_lf) {
	      in_header = 0;
	      ++i;
	      break;
	    }
	    was_lf = 1;
	  }
	  else
	    was_lf = 0;
	}
      }
      if (!in_header) {
	for (; lines > 0 && i < n; ++i) {
	  if (x[i] == '\n')
	      --lines;
	}
      }
      n = i;
    }
    if (!n) return 0;
    qmail_put(qq,x,n);
    substdio_SEEK(ss,n);
  }
}
