#ifndef GENERIC_SEARCH_H
#define GENERIC_SEARCH_H

#include <stdbool.h>
#include "generic_item.h"

typedef struct query_ Query;

Query* parse_query_file(const char *filename);
void query_free(Query *q);
bool query_avaliar_item(const ITEM *item, const Query *q, double *out_similarity);
const char* query_get_order_key(const Query *q, bool *out_desc);
bool query_order_is_similarity(const Query *q);
void query_get_group_keys(const Query *q, char *out_k1, char *out_k2, size_t max_len);
void query_get_cross_keys(const Query *q, char *out_k1, char *out_k2, size_t max_len);
bool query_has_group(const Query *q);
bool query_has_cross(const Query *q);

#endif
