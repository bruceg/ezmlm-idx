#ifndef SURFPCS_H
#define SURFPCS_H

#include "uint32.h"

typedef struct {
  uint32 seed[32];
  uint32 sum[8];
  uint32 in[12];
  int todo;
} surfpcs;

#define SURFPCS_LEN 32

extern void surfpcs_init(surfpcs *s,const uint32 k[32]);
extern void surfpcs_add(surfpcs *s,const char *x,unsigned int n);
extern void surfpcs_addlc(surfpcs *s,const char *x,unsigned int n);
extern void surfpcs_out(surfpcs *s,unsigned char h[32]);

#endif
