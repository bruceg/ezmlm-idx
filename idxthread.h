#ifndef IDXTHREAD_H
#define IDXTHREAD_H

#include "idx.h"

/* threading */
extern void idx_mkthread(msgentry **pmsgtable,
			 subentry **psubtable,
			 authentry **pauthtable,
			 unsigned long msg_from,
			 unsigned long msg_to,
			 unsigned long msg_master,
			 unsigned long msg_latest,
			 int locked,
			 const char *fatal);
extern void idx_mkthreads(msgentry **pmsgtable,
			  subentry **psubtable,
			  authentry **pauthtable,
			  dateentry **pdatetable,
			  unsigned long msg_from,
			  unsigned long msg_to,
			  unsigned long msg_latest,
			  int locked,
			  const char *fatal);
extern void idx_mklist(msgentry **pmsgtable,
		       subentry **psubtable,
		       authentry **pauthtable,
		       unsigned long msg_from,
		       unsigned long msg_to,
		       const char *fatal);
extern void idx_destroythread(msgentry *msgtable,
			      subentry *subtable,
			      authentry *authtable);

#endif
