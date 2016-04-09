/* Public domain, from daemontools-0.76. */

#ifndef GEN_ALLOC_DEFS_H
#define GEN_ALLOC_DEFS_H

#define GEN_ALLOC_ready(ta,type,field,len,a,i,n,x,base,ta_ready) \
void ta_ready(ta *x,unsigned int n) \
{ unsigned int i; \
  if (x->field) { \
    i = x->a; \
    if (n > i) { \
      x->a = base + n + (n >> 3); \
      alloc_re((void*)&x->field,i * sizeof(type),x->a * sizeof(type)); \
    } \
    return; } \
  x->len = 0; \
  x->field = (type *) alloc((x->a = n) * sizeof(type)); \
}

#define GEN_ALLOC_readyplus(ta,type,field,len,a,i,n,x,base,ta_rplus) \
void ta_rplus(ta *x,unsigned int n) \
{ unsigned int i; \
  if (x->field) { \
    i = x->a; n += x->len; \
    if (n > i) { \
      x->a = base + n + (n >> 3); \
      alloc_re((void*)&x->field,i * sizeof(type),x->a * sizeof(type)); \
    } \
    return; } \
  x->len = 0; \
  x->field = (type *) alloc((x->a = n) * sizeof(type)); \
}

#define GEN_ALLOC_append(ta,type,field,len,a,i,n,x,base,ta_rplus,ta_append) \
void ta_append(ta *x,type i) \
{ ta_rplus(x,1); x->field[x->len++] = i; }

#endif
