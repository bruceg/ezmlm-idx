#ifndef CONSTMAP_H
#define CONSTMAP_H

typedef unsigned long constmap_hash;

struct constmap {
  unsigned int num;
  constmap_hash mask;
  constmap_hash *hash;
  int *first;
  int *next;
  const char **input;
  int *inputlen;
} ;

extern int constmap_init(struct constmap *cm,const char *s,int len,int flagcolon);
extern void constmap_free(struct constmap *cm);
extern const char *constmap(struct constmap *cm,const char *s,int len);
extern const char *constmap_get(struct constmap *cm,unsigned int idx);
extern int constmap_index(const struct constmap *cm,const char *s,int len);
#endif
