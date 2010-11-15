#include "hasattribute.h"

const char FATAL[] = "ezmlm-sub: fatal: ";
const char USAGE[] =
"ezmlm-sub: usage: ezmlm-sub [-mMvV] [-h hash] [-n] dir [subdir] [box@domain [name]] ...";

extern void subunsub_main(int submode,int argc,char **argv) __attribute__((noreturn));

int main(int argc,char **argv)
{
  subunsub_main(1,argc,argv);
}
