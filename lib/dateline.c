#include "fmt.h"
#include "yyyymm.h"
#include "stralloc.h"
#include "cgi.h"

void dateline(stralloc *dt, unsigned long d)
/* converts yyyymm from unsigned long d to text dt */
{
  char strnum[FMT_ULONG];
  const char *mo;
  switch (d % 100) {
    case 1: mo = "January"; break;
    case 2: mo = "February"; break;
    case 3: mo = "March"; break;
    case 4: mo = "April"; break;
    case 5: mo = "May"; break;
    case 6: mo = "June"; break;
    case 7: mo = "July"; break;
    case 8: mo = "August"; break;
    case 9: mo = "September"; break;
    case 10: mo = "October"; break;
    case 11: mo = "November"; break;
    case 12: mo = "December"; break;
    case 0: mo = "????"; break;
    default: cgierr("I don't know any month > 12",
		"","");
  }
  stralloc_copys(dt,mo);
  stralloc_cats(dt," ");
  if ((d/100)) {
    stralloc_catb(dt,strnum,fmt_ulong(strnum,d/100));
  } else
    stralloc_cats(dt,"????");
}


