/*$Id$*/

#include "yyyymm.h"

unsigned int date2yyyymm(const char *s)
/* expects a qmail date string s and returns yyyymm */
/* if there are problems, it returns 0. If there is no terminating char */
/* we may segfault if the syntax is bad. Assure that the ';' is there   */
/* or add '\0' */
{
  unsigned int mo;
  unsigned int year;	/* must hold yyyymm - ok to year 65K */
  char ch,ch1,ch2;

/* jan feb mar apr may jun jul aug sep oct nov dec */
/* - strictly qmail datefmt dependent*/
  for (;;s++) {
    ch = *s;
    if (ch != ' ' && (ch < '0' || ch > '9')) break;
  }
  mo = 0;
  if (!(ch = *(s++))) return 0;
  if (ch >= 'a')  ch -= ('a' - 'A');	/* toupper */
  if (!(ch1 = *(s++))) return 0;	/* rfc822 hrds are case-insens */
  if (ch1 >= 'a')  ch1 -= ('a' - 'A');
  if (!(ch2 = *(s++))) return 0;
  if (ch2 >= 'a')  ch2 -= ('a' - 'A');

  switch (ch) {
    case 'J':
	if (ch1 == 'A' && ch2 == 'N') { mo = 1; break; }
	if (ch1 == 'U') {
	  if (ch2 == 'N') mo = 6;
	  else if (ch2 == 'L') mo = 7;
	}
	break;
    case 'F': if (ch1 == 'E' && ch2 == 'B') mo = 2; break;
    case 'A':
	if (ch1 == 'P' && ch2 == 'R') mo = 4;
	else if (ch1 == 'U' && ch2 == 'G') mo = 8;
	break;
    case 'M':
	if (ch1 != 'A') break;
	if (ch2 == 'R') mo = 3;
	else if (ch2 == 'Y') mo = 5;
	break;
    case 'S': if (ch1 == 'E' && ch2 == 'P') mo = 9; break;
    case 'O': if (ch1 == 'C' && ch2 == 'T') mo = 10; break;
    case 'N': if (ch1 == 'O' && ch2 == 'V') mo = 11; break;
    case 'D': if (ch1 == 'E' && ch2 == 'C') mo = 12; break;
    default:
	break;
  }
  if (!mo || *(s++) != ' ')
    return 0L;		/* mo true means s[0-2] valid */
  year = 0L;
  for (;;) {
    unsigned char chy;
    chy = (unsigned char) *(s++);
    if (chy < '0' || chy > '9') {
      if (year) break;
      else return 0;
    }
    year = year * 10 + (chy - '0');
  }
  return year * 100 + mo;
}


