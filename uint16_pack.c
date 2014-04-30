/* Public domain, from djbdns-1.05. */

#include "uint16.h"

void uint16_pack(char s[2],uint16 u)
{
  s[0] = u & 255;
  s[1] = u >> 8;
}

void uint16_pack_big(char s[2],uint16 u)
{
  s[1] = u & 255;
  s[0] = u >> 8;
}
