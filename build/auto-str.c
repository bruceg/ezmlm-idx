#include "str.h"
#include "subfd.h"
#include "substdio.h"
#include "readwrite.h"
#include "exit.h"

void subputs(const char *s)
{
  if (substdio_puts(subfdout,s) == -1) _exit(111);
}

void subputsbin(const char *s)
{
  char octal[5];
  unsigned char ch;
  while ((ch = *s++) != 0) {
    octal[4] = 0;
    octal[3] = '0' + (ch & 7); ch >>= 3;
    octal[2] = '0' + (ch & 7); ch >>= 3;
    octal[1] = '0' + (ch & 7);
    octal[0] = '\\';
    subputs(octal);
  }
}

int main(int argc, char **argv)
{
  char *name;
  char *env;
  char value[256];
  int len;

  name = argv[1];
  if (!name) _exit(100);
  env = argv[2];
  if ((len = substdio_get(subfdin,value,sizeof value - 1)) <= 0)
    _exit(100);
  value[len] = 0;
  value[str_chr(value,'\n')] = 0;

  if (env) {
    subputs("#include \"env.h\"\n"
	    "const char *");
    subputs(name);
    subputs("(void)\n"
	    "{\n"
	    "  static const char *env = 0;\n"
	    "  if (env == 0)\n"
	    "    if ((env = env_get(\"");
    subputs(env);
    subputs("\")) == 0)\n"
	    "      env = \"");
    subputsbin(value);
    subputs("\";\n"
	    "  return env;\n"
	    "}\n");
  }
  else {
    subputs("const char ");
    subputs(name);
    subputs("[] = \"");
    subputsbin(value);
    subputs("\";\n");
  }
  if (substdio_flush(subfdout) == -1) _exit(111);
  return 0;
  (void)argc;
}
