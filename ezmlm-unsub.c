const char FATAL[] = "ezmlm-unsub: fatal: ";
const char USAGE[] =
"ezmlm-unsub: usage: ezmlm-unsub [-h hash] [-HmMnNvV] dir [subdir] [box@domain ...]";

extern void subunsub_main(int submode,int argc,char **argv);

void main(int argc,char **argv)
{
  subunsub_main(0,argc,argv);
}
