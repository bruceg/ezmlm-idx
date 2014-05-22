#ifndef GETCONFOPT__H
#define GETCONFOPT__H

struct option;
struct stralloc;

struct option_type
{
  int needsarg;
  int (*parse)(struct option*,const char*);
  void (*fetch)(struct option*);
};

extern const struct option_type opt_copy_flag;
extern const struct option_type opt_counter;
extern const struct option_type opt_cstr;
extern const struct option_type opt_cstr_flag;
extern const struct option_type opt_flag;
extern const struct option_type opt_str;
extern const struct option_type opt_ulong;
extern const struct option_type opt_ulong_flag;
extern const struct option_type opt_version;

struct option
{
  union
  {
    int *flag;
    unsigned long *ulong;
    const char **cstr;
    struct stralloc *str;
  } var;
  const char ch;
  const struct option_type *type;
  const union
  {
    int flag;
    unsigned long ulong;
    const char *cstr;
  } value;
  const char *filename;
};

#define OPT_COPY_FLAG(VAR,CH) {{.str=&VAR},CH,&opt_copy_flag,{0},0}
#define OPT_COUNTER(VAR,CH,VALUE,FILENAME) {{.flag=&VAR},CH,&opt_counter,{.flag=VALUE},FILENAME}
#define OPT_CSTR(VAR,CH,FILENAME) {{.cstr=&VAR},CH,&opt_cstr,{0},FILENAME}
#define OPT_CSTR_FLAG(VAR,CH,VALUE,FILENAME) {{.cstr=&VAR},CH,&opt_cstr_flag,{.cstr=VALUE},FILENAME}
#define OPT_FLAG(VAR,CH,VALUE,FILENAME) {{.flag=&VAR},CH,&opt_flag,{.flag=VALUE},FILENAME}
#define OPT_STR(VAR,CH,FILENAME) {{.str=&VAR},CH,&opt_str,{0},FILENAME}
#define OPT_ULONG(VAR,CH,FILENAME) {{.ulong=&VAR},CH,&opt_ulong,{0},FILENAME}
#define OPT_ULONG_FLAG(VAR,CH,VALUE,FILENAME) {{.ulong=&VAR},CH,&opt_ulong_flag,{.ulong=VALUE},FILENAME}
#define OPT_END \
	{{0},'v',&opt_version,{0},0},\
	{{0},'V',&opt_version,{0},0},\
	{{0},0,0,{0},0}

extern int getconfopt(int argc, char *argv[], struct option *options,
		      int dochdir,const char **dir);

#endif
