#ifndef SURFPCS_H
#define SURFPCS_H

#include "uint32.h"

typedef struct {
  uint32 seed[32];
  uint32 sum[8];
  uint32 out[8];
  uint32 in[12];
  int todo;
} surfpcs;

#define SURFPCS_LEN 32

extern void surfpcs_init();
extern void surfpcs_add();
extern void surfpcs_out();

#endif
