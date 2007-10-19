/* XXX: this needs testing */

#include "surf.h"
#include "surfpcs.h"

void surfpcs_init(surfpcs *s,const uint32 k[32])
{
  int i;
  for (i = 0;i < 32;++i) s->seed[i] = k[i];
  for (i = 0;i < 8;++i) s->sum[i] = 0;
  for (i = 0;i < 12;++i) s->in[i] = 0;
  s->todo = 0;
}

static const uint32 littleendian[8] = {
  50462976, 117835012, 185207048, 252579084,
  319951120, 387323156, 454695192, 522067228
} ;
#define end ((unsigned char *) &littleendian)

#define data ((unsigned char *) s->in)
#define outdata ((unsigned char *) s->out)

void surfpcs_add(surfpcs *s,const unsigned char *x,unsigned int n)
{
  int i;
  while (n--) {
    data[end[s->todo++]] = *x++;
    if (s->todo == 32) {
      s->todo = 0;
      if (!++s->in[8])
        if (!++s->in[9])
          if (!++s->in[10])
            ++s->in[11];
      surf(s->out,s->in,s->seed);
      for (i = 0;i < 8;++i)
	s->sum[i] += s->out[i];
    }
  }
}

void surfpcs_out(surfpcs *s,unsigned char h[32])
{
  int i;
  surfpcs_add(s,(const unsigned char*)".",1);
  while (s->todo) surfpcs_add(s,(const unsigned char*)"",1);
  for (i = 0;i < 8;++i) s->in[i] = s->sum[i];
  for (;i < 12;++i) s->in[i] = 0;
  surf(s->out,s->in,s->seed);
  for (i = 0;i < 32;++i) h[i] = outdata[end[i]];
}
