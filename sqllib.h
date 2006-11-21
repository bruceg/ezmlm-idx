/*$Id$*/
#ifndef SQLLIB_H
#define SQLLIB_H

struct sqlinfo
{
  const char *host;
  unsigned long port;
  const char *db;
  const char *user;
  const char *pw;
  const char *table;
  void *conn;
};

extern const char *parsesql(const char *subdir,
			    struct sqlinfo *info);

#endif
