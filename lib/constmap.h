#ifndef CONSTMAP_H
#define CONSTMAP_H

typedef unsigned long constmap_hash;

struct constmap_entry {
  const char *input;
  int inputlen;
  constmap_hash hash;
  int next;
};

struct constmap {
  unsigned int num;
  constmap_hash mask;
  int *first;
  struct constmap_entry *entries;
} ;

extern int constmap_init(struct constmap *cm,const char *s,int len,int splitchar);
extern void constmap_free(struct constmap *cm);
extern const char *constmap(struct constmap *cm,const char *s,int len);
extern const char *constmap_get(struct constmap *cm,unsigned int idx);
extern int constmap_index(const struct constmap *cm,const char *s,int len);
#endif
