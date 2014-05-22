#include "sys/hasattribute.h"

const char FATAL[] = "ezmlm-unsub: fatal: ";
const char USAGE[] =
"ezmlm-unsub: usage: ezmlm-unsub [-HmMnNvV] [-h hash] [-t tag] dir [subdir] [box@domain ...]";

extern void subunsub_main(int submode,int argc,char **argv) __attribute__((noreturn));

int main(int argc,char **argv)
{
  subunsub_main(0,argc,argv);
}
