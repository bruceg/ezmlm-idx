#include "cookie.h"
#include "str.h"
#include "uint32.h"
#include "surfpcs.h"

void cookie(hash,key,keylen,date,addr,action)
char *hash;
char *key;
unsigned int keylen;
char *date;
char *addr;
char *action;
{
  surfpcs s;
  uint32 seed[32];
  unsigned char out[32];
  int i;
  int j;

/*
step 1: create seed from key. note that this doesn't have to be
cryptographic; it simply has to avoid destroying the user's entropy.
if speed turns out to be a problem, switch to a CRC.
*/
  for (i = 0;i < 32;++i) seed[i] = 0;
  for (j = 0;j < 4;++j) {
    surfpcs_init(&s,seed);
    surfpcs_add(&s,key,keylen);
    surfpcs_out(&s,out);
    for (i = 0;i < 32;++i) seed[i] = (seed[i] << 8) + out[i];
  }

/*
step 2: apply SURF.
*/
  surfpcs_init(&s,seed);
  surfpcs_add(&s,date,str_len(date) + 1);
  surfpcs_add(&s,addr,str_len(addr) + 1);
  surfpcs_add(&s,action,1);
  surfpcs_out(&s,out);

/*
step 3: extract a readable cookie from the SURF output.
*/
  for (i = 0;i < 20;++i) 
    hash[i] = 'a' + (out[i] & 15);
}
