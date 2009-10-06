#ifndef SUB_SQL_H
#define SUB_SQL_H

#include "subdb.h"

extern const char sql_sub_table_defn[];
extern const char sql_slog_table_defn[];
extern const char sql_cookie_table_defn[];
extern const char sql_mlog_table_defn[];

extern int sql_table_exists(struct subdbinfo *info,
			    const char *name);
extern const char *sql_create_table(struct subdbinfo *info,
				    const char *defn);
extern const char *sql_drop_table(struct subdbinfo *info,
				  const char *name);

extern const char *sub_sql_mktab(struct subdbinfo *info);
extern const char *sub_sql_rmtab(struct subdbinfo *info);

#endif
