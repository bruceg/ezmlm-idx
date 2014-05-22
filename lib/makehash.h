#ifndef MAKEHASH_H
#define MAKEHASH_H

#define HASHLEN 20

extern void makehash(const char *indata,unsigned int inlen,char *hash);
extern void mkauthhash(const char *s,unsigned int len,char *h);

#endif

