#include "sys/hasattribute.h"

const char FATAL[] = "ezmlm-sub: fatal: ";
const char USAGE[] =
"ezmlm-sub: usage: ezmlm-sub [-HmMnNvV] [-h hash] [-t tag] dir [subdir] [box@domain [name]] ...";

extern void subunsub_main(int submode,int argc,char **argv) __attribute__((noreturn));

int main(int argc,char **argv)
{
  subunsub_main(1,argc,argv);
}
