#include "stralloc.h"
#include "byte.h"
#include "surf.h"
#include "uint32.h"
#include "makehash.h"
#include "die.h"
#include "idx.h"
#include "surfpcs.h"

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


