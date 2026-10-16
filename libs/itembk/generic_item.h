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
typedef struct generic_item_ Item;

// Leitura, ciclo de vida e impressão
Item** read_csv_to_items(const char *filename, int *out_num_rows);
void item_free(Item *item);
void item_print(const Item *item);
const char* dtype_to_string(DataType type);

// Getters da struct Item
int item_get_num_fields(const Item *item);
const char* item_get_key(const Item *item, int index);
int item_get_column_index(const Item *item, const char *col_name);
DataType item_get_value_type(const Item *item, int index);

bool item_get_value_bool(const Item *item, int index);
int item_get_value_int(const Item *item, int index);
float item_get_value_float(const Item *item, int index);
bool item_get_value_as_double(const Item *item, int index, double *out_val);
const char* item_get_value_string(const Item *item, int index);

#endif // GENERIC_ITEM_H