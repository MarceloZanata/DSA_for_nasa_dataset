#ifndef GENERIC_SEARCH_H
#define GENERIC_SEARCH_H

#include "generic_item.h"

typedef struct query_ Query;

Query* parse_query_file(const char *filename);
Item** execute_query(Item **dataset, int total_rows, const Query *q, int *out_count);
void query_free(Query *q);

#endif // GENERIC_SEARCH_H