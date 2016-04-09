/* Public domain, from daemontools-0.76. */

#include <stdlib.h>
#include "alloc.h"
#include "die.h"

#define ALIGNMENT 16 /* XXX: assuming that this alignment is enough */
#define SPACE 2048 /* must be multiple of ALIGNMENT */

typedef union { char irrelevant[ALIGNMENT]; double d; } aligned;
static aligned realspace[SPACE / ALIGNMENT];
#define space ((char *) realspace)
static unsigned int avail = SPACE; /* multiple of ALIGNMENT; 0<=avail<=SPACE */

/*@null@*/void *alloc_nodie(unsigned int n)
{
  n = ALIGNMENT + n - (n & (ALIGNMENT - 1)); /* XXX: could overflow */
  if (n <= avail) { avail -= n; return space + avail; }
  return malloc(n);
}

void *alloc(unsigned int n)
{
  char *x;
  x = alloc_nodie(n);
  if (!x)
    die_nomem();
  return x;
}

void alloc_free(void *x)
{
  if (x >= (void*)space)
    if (x < (void*)(space + SPACE))
      return; /* XXX: assuming that pointers are flat */
  free(x);
}
