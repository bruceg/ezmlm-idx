#ifndef COOKIE_H
#define COOKIE_H

#define COOKIE 20

extern void cookie(char *hash,
		   const char *key,
		   unsigned int keylen,
		   const char *date,
		   const char *addr,
		   const char *action);

#endif
