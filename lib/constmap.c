#include "constmap.h"
#include "alloc.h"
#include "case.h"

static constmap_hash hash(const char *s,int len)
{
  unsigned char ch;
  constmap_hash h;
  h = 5381;
  while (len > 0) {
    ch = *s++ - 'A';
    if (ch <= 'Z' - 'A') ch += 'a' - 'A';
    h = ((h << 5) + h) ^ ch;
    --len;
  }
  return h;
}

/* Returns index of string in constmap. 1 = first string, 2 = second ... */
/* 0 not found. Use for commands */ 
int constmap_index(const struct constmap *cm,const char *s,int len)
{
  constmap_hash h;
  struct constmap_entry *e;
  int pos;
  h = hash(s,len);
  pos = cm->first[h & cm->mask];
  while (pos != -1) {
    e = &cm->entries[pos];
    if (h == e->hash)
      if (len == e->inputlen)
        if (!case_diffb(e->input,len,s))
	  return pos + 1;
    pos = e->next;
  }
  return 0;
}

/* returns pointer to sz of string with index "idx". 1 = first, 2 = second...*/
const char *constmap_get(struct constmap *cm,unsigned int idx)
{
  if (idx <= 0 || idx > cm->num)
    return 0;
  else
    return cm->entries[idx-1].input;
}

const char *constmap(struct constmap *cm,const char *s,int len)
{
  constmap_hash h;
  struct constmap_entry *e;
  int pos;
  h = hash(s,len);
  pos = cm->first[h & cm->mask];
  while (pos != -1) {
    e = &cm->entries[pos];
    if (h == e->hash)
      if (len == e->inputlen)
        if (!case_diffb(e->input,len,s))
	  return e->input + e->inputlen + 1;
    pos = e->next;
  }
  return 0;
}

void constmap_init(struct constmap *cm,const char *s,int len,int splitchar)
/* if splitchar is set, we process only the stuff before that character
 * on each line. Otherwise, it's the entire line. Still, the entire line
 * is stored! */
{
  int i;
  int j;
  int k;
  int pos;
  constmap_hash h;
  struct constmap_entry *e;
 
  cm->num = 0;
  for (j = 0;j < len;++j) if (!s[j]) ++cm->num;
 
  h = 64;
  while (h && (h < cm->num)) h += h;
  cm->mask = h - 1;
 
  cm->first = (int *) alloc(sizeof(int) * h);
  cm->entries = alloc(cm->num * sizeof *cm->entries);
  for (h = 0;h <= cm->mask;++h)
    cm->first[h] = -1;
  pos = 0;
  i = 0;
  for (j = 0;j < len;++j)
    if (!s[j]) {
      k = j - i;
      if (splitchar) {
        for (k = i;k < j;++k)
          if (s[k] == splitchar)
            break;
        if (k >= j) { i = j + 1; continue; }
        k -= i;
      }
      h = hash(s + i,k);
      e = &cm->entries[pos];
      e->input = s + i;
      e->inputlen = k;
      e->hash = h;
      h &= cm->mask;
      e->next = cm->first[h];
      cm->first[h] = pos;
      ++pos;
      i = j + 1;
    }
}

void constmap_free(struct constmap *cm)
{
  alloc_free(cm->entries);
  alloc_free(cm->first);
}
