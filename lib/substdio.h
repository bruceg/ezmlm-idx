#ifndef SUBSTDIO_H
#define SUBSTDIO_H

#include <sys/types.h>
typedef ssize_t (*substdio_fn)();

typedef struct substdio {
  char *x;
  int p;
  int n;
  int fd;
  substdio_fn op;
} substdio;

#define SUBSTDIO_FDBUF(op,fd,buf,len) { (buf), 0, (len), (fd), (op) }

extern void substdio_fdbuf(substdio *s,substdio_fn op,int fd,char *buf,int len);

extern int substdio_flush(substdio *s);
extern int substdio_put(substdio *s,const char *buf,int len);
extern int substdio_bput(substdio *s,const char *buf,int len);
extern int substdio_putflush(substdio *s,const char *buf,int len);
extern int substdio_puts(substdio *s,const char *buf);
extern int substdio_bputs(substdio *s,const char *buf);
extern int substdio_putsflush(substdio *s,const char *buf);

extern int substdio_get(substdio *s,char *buf,int len);
extern int substdio_bget(substdio *s,char *buf,int len);
extern int substdio_feed(substdio *s);

extern const char *substdio_peek(substdio *s);
extern void substdio_seek(substdio *s,int len);

#define substdio_fileno(s) ((s)->fd)

#define SUBSTDIO_INSIZE 8192
#define SUBSTDIO_OUTSIZE 8192

#define substdio_PEEK(s) ( (s)->x + (s)->n )
#define substdio_SEEK(s,len) ( ( (s)->p -= (len) ) , ( (s)->n += (len) ) )

#define substdio_BPUTC(s,c) \
  ( ((s)->n != (s)->p) \
    ? ( (s)->x[(s)->p++] = (c), 0 ) \
    : substdio_bput((s),&(c),1) \
  )

extern int substdio_copy(substdio *ssout,substdio *ssin);

#endif
