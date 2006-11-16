/*$Id$*/
#ifndef SQLLIB_H
#define SQLLIB_H

struct sqlinfo
{
  const char *host;
  const char *port;
  const char *db;
  const char *user;
  const char *pw;
  const char *table;
};

extern const char *parsesql(const char *dir,
			    const char *subdir,
			    struct sqlinfo *info);

#endif
