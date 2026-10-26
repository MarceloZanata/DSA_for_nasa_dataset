#ifndef GENERIC_ITEM_H
#define GENERIC_ITEM_H

#include <stdio.h>
#include <stdbool.h>

typedef enum {
    TYPE_NULL,
    TYPE_BOOL,
    TYPE_INT,
    TYPE_FLOAT,
    TYPE_STRING
} DataType;

typedef struct value_ Value;
typedef struct csv_header_ CSVHeader;
typedef struct generic_item_ ITEM;

const char* dtype_to_string(DataType type);
DataType infer_single_type(const char *str);
DataType promote_type(DataType current_type, DataType new_type);
Value parse_value_with_type(const char *raw_text, DataType col_type);

char* csv_ler_linha(FILE *file);

CSVHeader* csv_header_ler(FILE *file);
void csv_header_free(CSVHeader *header);
int csv_header_get_num_cols(const CSVHeader *header);
const char* csv_header_get_col_name(const CSVHeader *header, int idx);
void csv_header_escrever(FILE *file, const CSVHeader *header);

ITEM* item_criar_de_linha(const char *linha, const CSVHeader *header);
ITEM* item_retain(ITEM *item);
void item_free(ITEM *item);
void item_apagar(ITEM **item);
int item_get_chave(const ITEM *item);

void item_print(const ITEM *item);
int item_get_num_fields(const ITEM *item);
const char* item_get_key(const ITEM *item, int index);
int item_get_column_index(const ITEM *item, const char *col_name);
DataType item_get_value_type(const ITEM *item, int index);
bool item_get_value_bool(const ITEM *item, int index);
long long item_get_value_int(const ITEM *item, int index);
double item_get_value_float(const ITEM *item, int index);
bool item_get_value_as_double(const ITEM *item, int index, double *out_val);
const char* item_get_value_string(const ITEM *item, int index);

void item_set_similarity(ITEM *item, double sim);
double item_get_similarity(const ITEM *item);

void item_escrever_csv(FILE *file, const ITEM *item);
ITEM** read_csv_to_items(const char *filename, int *out_num_rows);

#endif
