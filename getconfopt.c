#include "config.h"
#include "die.h"
#include "getconf.h"
#include "getconfopt.h"
#include "str.h"

static int count(int argc,char *argv[],struct option *options)
{
  int i;
  int j;
  int ch;
  struct option *o;

  /* First pass: find the end of the options, to find DIR. */
  for (i = 1; i < argc && argv[i][0] == '-'; ++i) {
    if (argv[i][1] == 0
	|| (argv[i][1] == '-' && argv[i][2] == 0))
      return i + 1;
    for (j = 1; argv[i][j] != 0; ++j) {
      ch = argv[i][j];
      for (o = options; o->type != 0; ++o) {
	if (ch == o->ch) {
	  if (o->type->needsarg) {
	    /* Look for -oVALUE */
	    if (argv[i][j+1] != 0)
	      ;
	    /* Look for -o VALUE */
	    else if (i + 1 < argc)
	      ++i;
	    else
	      die_usage();
	    j = str_len(argv[i]) - 1;
	  }
	  break;
	}
      }
      if (o->type == 0)
	die_usage();
    }
  }
  return i;
}

static void init(struct option *options)
{
  struct option *o;
  for (o = options; o->type != 0; ++o)
    if (o->filename && o->type->fetch)
      o->type->fetch(o);
}

static void parse(int argc,char *argv[],struct option *options)
{
  int i;
  int j;
  int ch;
  struct option *o;

  /* Second pass: actually parse the options. */
  for (i = 1; i < argc && argv[i][0] == '-'; ++i) {
    if (argv[i][1] == 0
	|| (argv[i][1] == '-' && argv[i][2] == 0))
      break;
    for (j = 1; argv[i][j] != 0; ++j) {
      ch = argv[i][j];
      for (o = options; o->type != 0; ++o) {
	if (ch == o->ch) {
	  if (!o->type->needsarg)
	    (void)o->type->parse(o,0);
	  else {
	    /* Look for -oVALUE */
	    if (argv[i][j+1] != 0) {
	      if (!o->type->parse(o,argv[i]+j+1))
		die_usage();
	    }
	    /* Look for -o VALUE */
	    else if (i + 1 < argc) {
	      ++i;
	      if (!o->type->parse(o,argv[i]))
		die_usage();
	    }
	    else
	      die_usage();
	    j = str_len(argv[i]) - 1;
	  }
	  break;
	}
      }
      if (o->type == 0)
	die_usage();
    }
  }
}

int getconfopt(int argc,char *argv[],struct option *options,
	       int dochdir,const char **dir)
{
  int optind;

  optind = count(argc,argv,options);
  if (dir != 0)
    *dir = argv[optind];
  if (dochdir > 0
      || (dochdir < 0 && argv[optind] != 0)) {
    startup(argv[optind++]);
  }
  init(options);
  parse(argc,argv,options);
  return optind;
}
