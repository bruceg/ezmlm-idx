/*$Id$*/

const char FATAL[] = "ezmlm-sub: fatal: ";
const char USAGE[] =
"ezmlm-sub: usage: ezmlm-sub [-mMvV] [-h hash] [-n] dir [subdir] [box@domain [name]] ...";

extern void subunsub_main(int submode,
			  const char *version,
			  int argc,char **argv);

void main(int argc,char **argv)
{
  subunsub_main(1,"ezmlm-sub version: ",argc,argv);
}
