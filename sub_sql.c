#include "die.h"
#include "stralloc.h"
#include "sub_sql.h"
#include "subdb.h"

static stralloc name;
static stralloc query;

static void make_name(struct subdbinfo *info,
		      const char *suffix1,
		      const char *suffix2)
{
  if (!stralloc_copys(&name,info->base_table)) die_nomem();
  if (!stralloc_cats(&name,suffix1)) die_nomem();
  if (!stralloc_cats(&name,suffix2)) die_nomem();
  if (!stralloc_0(&name)) die_nomem();
}

static const char *create_table(struct subdbinfo *info,
				const char *suffix1,
				const char *suffix2,
				const char *definition)
{
  make_name(info,suffix1,suffix2);
  if (sql_table_exists(info,name.s) > 0)
    return 0;
  if (!stralloc_copys(&query,"CREATE TABLE ")) die_nomem();
  if (!stralloc_cats(&query,name.s)) die_nomem();
  if (!stralloc_cats(&query," (")) die_nomem();
  if (!stralloc_cats(&query,definition)) die_nomem();
  if (!stralloc_cats(&query,")")) die_nomem();
  if (!stralloc_0(&query)) die_nomem();
  return sql_create_table(info,query.s);
}

static const char *create_table_set(struct subdbinfo *info,
				    const char *suffix,
				    int do_mlog)
{
  const char *r;

  /* Address table */
  if ((r = create_table(info,suffix,"",sql_sub_table_defn)) != 0)
    return r;
  /* Subscription log table. */
  if ((r = create_table(info,suffix,"_slog",sql_slog_table_defn)) != 0)
    return r;

  if (do_mlog) {
    /* main list inserts a cookie here. Sublists check it */
    if ((r = create_table(info,suffix,"_cookie",sql_cookie_table_defn)) != 0)
      return r;

    /* main and sublist log here when the message is done done=0 for
     * arrived, done=4 for sent, 5 for receit. */
    if ((r = create_table(info,suffix,"_mlog",sql_mlog_table_defn)) != 0)
      return r;
  }

  return 0;
}

const char *sub_sql_mktab(struct subdbinfo *info)
{
  const char *r;

  if ((r = create_table_set(info,"",1)) != 0)
    return r;
  if ((r = create_table_set(info,"_allow",0)) != 0)
    return r;
  if ((r = create_table_set(info,"_deny",0)) != 0)
    return r;
  if ((r = create_table_set(info,"_digest",1)) != 0)
    return r;
  if ((r = create_table_set(info,"_mod",0)) != 0)
    return r;
  return 0;
}

static const char *remove_table(struct subdbinfo *info,
				const char *suffix1,
				const char *suffix2)
{
  make_name(info,suffix1,suffix2);
  if (sql_table_exists(info,name.s) == 0)
    return 0;
  return sql_drop_table(info,name.s);
}

static const char *remove_table_set(struct subdbinfo *info,
				    const char *suffix)
{
  const char *r;
  if ((r = remove_table(info,suffix,"_mlog")) != 0
      || (r = remove_table(info,suffix,"_cookie")) != 0
      || (r = remove_table(info,suffix,"_slog")) != 0
      || (r = remove_table(info,suffix,"")) != 0)
    return r;
  return 0;
}

const char *sub_sql_rmtab(struct subdbinfo *info)
{
  const char *r;
  if ((r = remove_table_set(info,"")) != 0
      || (r = remove_table_set(info,"_allow")) != 0
      || (r = remove_table_set(info,"_deny")) != 0
      || (r = remove_table_set(info,"_digest")) != 0
      || (r = remove_table_set(info,"_mod")) != 0)
    return r;
  return 0;
}
