/* Public domain, from djbdns-1.05. */

int main()
{
  unsigned long u;
  u = 1;
  u += u; u += u; u += u; u += u; u += u; u += u; u += u; u += u;
  u += u; u += u; u += u; u += u; u += u; u += u; u += u; u += u;
  u += u; u += u; u += u; u += u; u += u; u += u; u += u; u += u;
  u += u; u += u; u += u; u += u; u += u; u += u; u += u; u += u;
  if (!u) _exit(1);
  _exit(0);
}
