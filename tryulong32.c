/* Public domain, from djbdns-1.05. */
/* As per http://cr.yp.to/djbdns/res-disaster.html */

int main()
{
  unsigned long u;
  u = 1;
  u += u; u += u; u += u; u += u; u += u; u += u; u += u; u += u;
  u += u; u += u; u += u; u += u; u += u; u += u; u += u; u += u;
  u += u; u += u; u += u; u += u; u += u; u += u; u += u; u += u;
  u += u; u += u; u += u; u += u; u += u; u += u; u += u; u += u;
  if (!u) _exit(0);
  _exit(1);
}
