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
  0x03020100, 0x07060504, 0x0b0a0908, 0x0f0e0d0c,
  0x13121110, 0x17161514, 0x1b1a1918, 0x1f1e1d1c
} ;
#define end ((unsigned char *) &littleendian)

#define data ((unsigned char *) s->in)
#define outdata ((unsigned char *) s->out)

void surfpcs_add(surfpcs *s,const char *x,unsigned int n)
{
  int i;
  uint32 out[8];
  while (n--) {
    data[end[s->todo++]] = *x++;
    if (s->todo == 32) {
      s->todo = 0;
      if (!++s->in[8])
        if (!++s->in[9])
          if (!++s->in[10])
            ++s->in[11];
      surf(out,s->in,s->seed);
      for (i = 0;i < 8;++i)
	s->sum[i] += out[i];
    }
  }
}

void surfpcs_addlc(surfpcs *s,const char *x,unsigned int n)
/* modified from surfpcs_add by case-independence and skipping ' ' & '\t' */
{
  unsigned char ch;
  int i;
  uint32 out[8];
  while (n--) {
    ch = *x++;
    if (ch == ' ' || ch == '\t') continue;
    if (ch >= 'A' && ch <= 'Z')
      ch += 'a' - 'A';
    data[end[s->todo++]] = ch;
    if (s->todo == 32) {
      s->todo = 0;
      if (!++s->in[8])
        if (!++s->in[9])
          if (!++s->in[10])
            ++s->in[11];
      surf(out,s->in,s->seed);
      for (i = 0;i < 8;++i)
	s->sum[i] += out[i];
    }
  }
}

void surfpcs_out(surfpcs *s,unsigned char h[32])
{
  int i;
  uint32 out[8];
  surfpcs_add(s,".",1);
  while (s->todo) surfpcs_add(s,"",1);
  for (i = 0;i < 8;++i) s->in[i] = s->sum[i];
  for (;i < 12;++i) s->in[i] = 0;
  surf(out,s->in,s->seed);
  for (i = 0;i < 32;++i) h[i] = ((unsigned char*)out)[end[i]];
}
