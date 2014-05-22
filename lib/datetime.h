#ifndef DATETIME_H
#define DATETIME_H

struct datetime {
  int hour;
  int min;
  int sec;
  int wday;
  int mday;
  int yday;
  int mon;
  int year;
} ;

typedef unsigned long datetime_sec;

extern void datetime_tai(struct datetime *dt,datetime_sec t);

#endif
