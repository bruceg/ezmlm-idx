#ifndef SUBHASH_H
#define SUBHASH_H

unsigned int subhashs(const char *s);
unsigned int subhashb(const char *s,long len);
#define subhashsa(SA) subhashb((SA)->s,(SA)->len)

#endif
