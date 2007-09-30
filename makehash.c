/*$Id$*/

#include "stralloc.h"
#include "byte.h"
#include "surf.h"
#include "uint32.h"
#include "makehash.h"
#include "die.h"
#include "idx.h"

typedef struct {
  uint32 seed[32];
  uint32 sum[8];
  uint32 out[8];
  uint32 in[12];
  int todo;
} surfpcs;

#define SURFPCS_LEN 32

static void surfpcs_init(surfpcs *s,const uint32 k[32])
{
  int i;
  for (i = 0;i < 32;++i) s->seed[i] = k[i];
  for (i = 0;i < 8;++i) s->sum[i] = 0;
  for (i = 0;i < 12;++i) s->in[i] = 0;
  s->todo = 0;
}

static uint32 littleendian[8] = {
  0x03020100, 0x07060504, 0x0b0a0908, 0x0f0e0d0c,
  0x13121110, 0x17161514, 0x1b1a1918, 0x1f1e1d1c
} ;
#define end ((unsigned char *) littleendian)

#define data ((unsigned char *) s->in)
#define outdata ((unsigned char *) s->out)

static void surfpcs_addlc(surfpcs *s,const char *x,unsigned int n)
	/* modified from Dan's surfpcs_add by skipping ' ' & '\t' and */
	/* case-independence */
{
  unsigned char ch;
  int i;
  while (n--) {
    ch = *x++;
    if (ch == ' ' || ch == '\t') continue;
    if (ch >= 'A' && ch <= 'Z')
      data[end[s->todo++]] = ch - 'A' + 'a';
    else
      data[end[s->todo++]] = ch;
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

static void surfpcs_out(surfpcs *s,unsigned char h[32])
{
  int i;
  surfpcs_addlc(s,".",1);
  while (s->todo) surfpcs_addlc(s,"",1);
  for (i = 0;i < 8;++i) s->in[i] = s->sum[i];
  for (;i < 12;++i) s->in[i] = 0;
  surf(s->out,s->in,s->seed);
  for (i = 0;i < 32;++i) h[i] = outdata[end[i]];
}

void makehash(const char *indata,unsigned int inlen,char *hash)
	/* makes hash[COOKIE=20] from stralloc *indata, ignoring case and */
	/* SPACE/TAB */
{
  unsigned char h[32];
  surfpcs s;
  uint32 seed[32];
  int i;

  for (i = 0;i < 32;++i) seed[i] = 0;
  surfpcs_init(&s,seed);
  surfpcs_addlc(&s,indata,inlen);
  surfpcs_out(&s,h);
  for (i = 0;i < 20;++i)
    hash[i] = 'a' + (h[i] & 15);
}

static stralloc dummy = {0};

void mkauthhash(const char *s,unsigned int len,char *h)
/* This is a string that should be the same for all messages from a given   */
/* author. Doesn't have to be the real rfc822 address. We look for a '@'    */
/* and grab everything up to the next '>', ' ', or ';'. We go back the same */
/* way, then take everything up to the '@' or the first '-'. The latter     */
/* avoids problems with posters that band their addresses.                  */
{
  unsigned int i,j,k,l;
  char ch;

  j = k = l = 0;
  i = byte_rchr(s,len,'@');
  if (i < len) {		/* if not then i=sa->len, j=k=l=0 */
    j = i;
    while (++j < len) {		/* if not found, then j=sa->len */
      ch = s[j];
      if (ch == '>' || ch == ' ' || ch == ';') break;
    }
    k = i;
    while (k > 0) {		/* k <= i */
      ch = s[--k];
      if (ch == '<' || ch == ' ' || ch == ';') break;
    }
    l = k;			/* k <= l <= i; */
    while (l < i && s[l] != '-') ++l;
    if (!stralloc_copyb(&dummy,s + k, l - k)) die_nomem();
    if (!stralloc_catb(&dummy,s + i, j - i)) die_nomem();
    makehash(dummy.s,dummy.len,h);
  } else			/* use entire line if no '@' found */
    makehash(s,len,h);
}


