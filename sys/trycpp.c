/* Public domain, from daemontools-0.76. */

int main()
{
#ifdef NeXT
  printf("nextstep\n"); exit(0);
#endif
  printf("unknown\n"); exit(0);
}
