/* $Id$*/

#include "stralloc.h"
#include "uint32.h"
#include "mime.h"
#include "strerr.h"
#include "errtxt.h"

static void die_nomem(fatal)
  char *fatal;
{
  strerr_die2x(111,fatal,ERR_NOMEM);
}

static unsigned char base64char[] =
   "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static unsigned int pos = 0;
static unsigned int i = 0;
static uint32 hold32;
static unsigned char *cpout;

static void addone(ch)
unsigned char ch;
{
 if (!(pos++))
    hold32 = (uint32) ch;
  else
    hold32 = (hold32 << 8) | ch;
  if (pos == 3) {
    *cpout++ = base64char[(hold32 >> 18) & 0x3f];
    *cpout++ = base64char[(hold32 >> 12) & 0x3f];
    *cpout++ = base64char[(hold32 >>  6) & 0x3f];
    *cpout++ = base64char[hold32 & 0x3f];
    if (++i == 18) {
      *cpout++ = '\n';
      i = 0;
    }
    pos = 0;
  }
}

static void dorest()
{
  switch (pos) {
    case 2:
      hold32 = hold32 << 2;
      *cpout++ = base64char[(hold32 >> 12) & 0x3f];
      *cpout++ = base64char[(hold32 >> 06) & 0x3f];
      *cpout++ = base64char[hold32 & 0x3f];
      *cpout++ = '=';
      break;
    case 1:
      hold32 = hold32 << 4;
      *cpout++ = base64char[(hold32 >> 06) & 0x3f];
      *cpout++ = base64char[hold32 & 0x3f];
      *cpout++ = '=';
      *cpout++ = '=';
      break;
    default:
      break;
  }
  *cpout++ = '\n';
}   

void encodeB(indata,n,outdata,control,fatal)
unsigned char *indata;
unsigned int n;
stralloc *outdata;
int control;	/* 1 = init, 2 = flush */
char *fatal;
	/* converts any character with the high order bit set to */
	/* base64. In: n chars of indata, out: stralloc outdata  */
	/* as '=' is not allowed within the block, we cannot flush after */
	/* each line, so we carry over data from call to call. The last  */
	/* call to encodeB should have control = 2 to do the flushing.   */
	/* control = 0 resets, and the routine starts out reset. */
{
  register unsigned char ch;

  if (control == 1) {
    pos = 0;
    i = 0;
  }
  if (!stralloc_copys(outdata,"")) die_nomem(fatal);
  if (!stralloc_ready(outdata,n*8/3 + n/72 + 5)) die_nomem(fatal);
  cpout = (unsigned char *) outdata->s;
  while (n--) {
    ch = *indata++;
    if (ch == '\n')
      addone('\r');
    addone(ch);
  }
  if (control == 2)
    dorest();
  outdata->len = (unsigned int) (cpout - (unsigned char *) outdata->s);
}

