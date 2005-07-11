#include "subfd.h"
#include "substdio.h"
#include "readwrite.h"
#include "exit.h"

void subputs(const char *s)
{
  if (substdio_puts(subfdout,s) == -1) _exit(111);
}

int main(int argc, char **argv)
{
  char *name;
  char *value;
  unsigned char ch;
  char octal[4];

  name = argv[1];
  if (!name) _exit(100);
  value = argv[2];
  if (!value) _exit(100);

  subputs("const char ");
  subputs(name);
  subputs("[] = \"\\\n");

  while ((ch = *value++) != 0) {
    subputs("\\");
    octal[3] = 0;
    octal[2] = '0' + (ch & 7); ch >>= 3;
    octal[1] = '0' + (ch & 7); ch >>= 3;
    octal[0] = '0' + (ch & 7);
    subputs(octal);
  }

  subputs("\\\n\";\n");
  if (substdio_flush(subfdout) == -1) _exit(111);
  return 0;
}
