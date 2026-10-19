#ifndef GENERIC_ITEM_H
#define GENERIC_ITEM_H

#include <stdbool.h>

typedef enum {
    TYPE_NULL,
    TYPE_INT,
    TYPE_FLOAT,
    TYPE_BOOL,
    TYPE_STRING
} DataType;

// Tipos opacos para encapsulamento
typedef struct value_ Value;
typedef struct generic_item_ ITEM;

// Leitura, ciclo de vida e impressão
ITEM** read_csv_to_items(const char *filename, int *out_num_rows);
void item_free(ITEM *item);
void item_print(const ITEM *item);
const char* dtype_to_string(DataType type);

// Getters da struct ITEM
int item_get_num_fields(const ITEM *item);
const char* item_get_key(const ITEM *item, int index);
int item_get_column_index(const ITEM *item, const char *col_name);
DataType item_get_value_type(const ITEM *item, int index);

bool item_get_value_bool(const ITEM *item, int index);
long long item_get_value_int(const ITEM *item, int index);   // int64
double item_get_value_float(const ITEM *item, int index);    // float64
bool item_get_value_as_double(const ITEM *item, int index, double *out_val);
const char* item_get_value_string(const ITEM *item, int index);

#endif // GENERIC_ITEM_H
