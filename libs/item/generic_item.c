#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include "generic_item.h"

#define MAX_LINE_LEN 2048
#define INITIAL_CAPACITY 10

struct value_ {
    DataType type;
    union {
        int i_val;
        float f_val;
        bool b_val;
        char *s_val;
    } data;
};

struct generic_item_ {
    int num_fields;
    char **keys;
    Value *values;
};

typedef struct {
    char **raw_fields;
} RawRow;

const char* dtype_to_string(DataType type) {
    switch (type) {
        case TYPE_INT:    return "int64";
        case TYPE_FLOAT:  return "float64";
        case TYPE_BOOL:   return "bool";
        case TYPE_STRING: return "object";
        case TYPE_NULL:   return "null";
        default:          return "unknown";
    }
}

char* trim_whitespace(char *str) {
    while (isspace((unsigned char)*str)) str++;
    if (*str == 0) return str;
    char *end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;
    end[1] = '\0';
    return str;
}

int parse_csv_line(char *line, char **out_fields, int max_fields) {
    int count = 0;
    char *ptr = line;
    char *start = ptr;

    while (count < max_fields) {
        if (*ptr == ',' || *ptr == '\0') {
            bool is_end = (*ptr == '\0');
            *ptr = '\0';
            
            // O trim_whitespace DEVE ser chamado em cada campo individual aqui!
            out_fields[count++] = strdup(trim_whitespace(start));
            
            if (is_end) break;
            start = ptr + 1;
        }
        ptr++;
    }
    return count;
}

DataType infer_single_type(const char *str) {
    if (str == NULL || *str == '\0') return TYPE_NULL;

    // Criamos uma cópia local ou ajustamos ponteiros para ignorar aspas nas pontas se existirem
    char cleaned[128];
    int len = strlen(str);
    int start = 0, end = len - 1;

    if (str[0] == '"' && str[end] == '"' && len >= 2) {
        start++;
        end--;
    }

    int j = 0;
    for (int i = start; i <= end; i++) {
        cleaned[j++] = str[i];
    }
    cleaned[j] = '\0';

    if (cleaned[0] == '\0') return TYPE_NULL;
    if (strcasecmp(cleaned, "true") == 0 || strcasecmp(cleaned, "false") == 0) return TYPE_BOOL;

    int has_decimal = 0;
    int i = 0;

    if (cleaned[0] == '-' || cleaned[0] == '+') i++;
    if (cleaned[i] == '\0') return TYPE_STRING;

    for (; cleaned[i] != '\0'; i++) {
        if (cleaned[i] == '.') {
            if (has_decimal) return TYPE_STRING;
            has_decimal = 1;
        } else if (!isdigit((unsigned char)cleaned[i])) {
            return TYPE_STRING;
        }
    }
    return has_decimal ? TYPE_FLOAT : TYPE_INT;
}

DataType promote_type(DataType current_type, DataType new_type) {
    if (new_type == TYPE_NULL) return current_type;
    if (current_type == TYPE_NULL) return new_type;
    if (current_type == new_type) return current_type;

    if ((current_type == TYPE_INT && new_type == TYPE_FLOAT) ||
        (current_type == TYPE_FLOAT && new_type == TYPE_INT)) {
        return TYPE_FLOAT;
    }
    return TYPE_STRING;
}

Value parse_value_with_type(const char *raw_text, DataType col_type) {
    Value val;
    val.type = col_type;

    if (raw_text == NULL || *raw_text == '\0') {
        val.type = TYPE_NULL;
        val.data.s_val = NULL;
        return val;
    }

    switch (col_type) {
        case TYPE_BOOL:
            val.data.b_val = (strcasecmp(raw_text, "true") == 0 || strcmp(raw_text, "1") == 0);
            break;
        case TYPE_INT:
            val.data.i_val = atoi(raw_text);
            break;
        case TYPE_FLOAT:
            val.data.f_val = (float)atof(raw_text);
            break;
        case TYPE_STRING:
        default:
            val.type = TYPE_STRING;
            val.data.s_val = strdup(raw_text);
            break;
    }
    return val;
}

void item_print(const Item *item) {
    if (!item) return;
    printf("{\n");
    for (int i = 0; i < item->num_fields; i++) {
        printf("  \"%-18s\": ", item->keys[i]);
        if (item->values[i].type == TYPE_NULL) {
            printf("%-15s", "null");
        } else if (item->values[i].type == TYPE_BOOL) {
            printf("%-15s", item->values[i].data.b_val ? "true" : "false");
        } else if (item->values[i].type == TYPE_INT) {
            printf("%-15d", item->values[i].data.i_val);
        } else if (item->values[i].type == TYPE_FLOAT) {
            printf("%-15.6f", item->values[i].data.f_val);
        } else {
            char buffer[64];
            snprintf(buffer, sizeof(buffer), "\"%s\"", item->values[i].data.s_val);
            printf("%-15s", buffer);
        }
        printf(" [%s]\n", dtype_to_string(item->values[i].type));
    }
    printf("}\n");
}

void item_free(Item *item) {
    if (!item) return;
    for (int i = 0; i < item->num_fields; i++) {
        free(item->keys[i]);
        if (item->values[i].type == TYPE_STRING) {
            free(item->values[i].data.s_val);
        }
    }
    free(item->keys);
    free(item->values);
    free(item);
}

Item** read_csv_to_items(const char *filename, int *out_num_rows) {
    FILE *file = fopen(filename, "r");
    if (!file) return NULL;

    char line[MAX_LINE_LEN];
    if (!fgets(line, sizeof(line), file)) {
        fclose(file);
        return NULL;
    }

    line[strcspn(line, "\r\n")] = 0;

    int num_cols = 0;
    for (char *p = line; *p; p++) {
        if (*p == ',') num_cols++;
    }
    num_cols++;

    char **headers = (char**) malloc(num_cols * sizeof(char*));
    parse_csv_line(line, headers, num_cols);

    int capacity = INITIAL_CAPACITY;
    int num_rows = 0;
    RawRow *raw_rows = (RawRow*) malloc(capacity * sizeof(RawRow));

    DataType *column_types = (DataType*) malloc(num_cols * sizeof(DataType));
    for (int i = 0; i < num_cols; i++) column_types[i] = TYPE_NULL;

    while (fgets(line, sizeof(line), file)) {
        line[strcspn(line, "\r\n")] = 0;
        if (strlen(line) == 0) continue;

        if (num_rows >= capacity) {
            capacity *= 2;
            raw_rows = (RawRow*) realloc(raw_rows, capacity * sizeof(RawRow));
        }

        raw_rows[num_rows].raw_fields = (char**) malloc(num_cols * sizeof(char*));
        parse_csv_line(line, raw_rows[num_rows].raw_fields, num_cols);

        for (int i = 0; i < num_cols; i++) {
            DataType cell_type = infer_single_type(raw_rows[num_rows].raw_fields[i]);
            column_types[i] = promote_type(column_types[i], cell_type);
        }
        num_rows++;
    }
    fclose(file);

    for (int i = 0; i < num_cols; i++) {
        if (column_types[i] == TYPE_NULL) column_types[i] = TYPE_STRING;
    }

    Item **items = (Item**) malloc(num_rows * sizeof(Item*));

    for (int r = 0; r < num_rows; r++) {
        Item *new_item = (Item*) malloc(sizeof(Item));
        new_item->num_fields = num_cols;
        new_item->keys = (char**) malloc(num_cols * sizeof(char*));
        new_item->values = (Value*) malloc(num_cols * sizeof(Value));

        for (int c = 0; c < num_cols; c++) {
            new_item->keys[c] = strdup(headers[c]);
            new_item->values[c] = parse_value_with_type(raw_rows[r].raw_fields[c], column_types[c]);
            free(raw_rows[r].raw_fields[c]);
        }
        free(raw_rows[r].raw_fields);
        items[r] = new_item;
    }

    for (int i = 0; i < num_cols; i++) free(headers[i]);
    free(headers);
    free(raw_rows);
    free(column_types);

    *out_num_rows = num_rows;
    return items;
}

// Getters
int item_get_num_fields(const Item *item) { return item ? item->num_fields : 0; }
const char* item_get_key(const Item *item, int index) { return (!item || index < 0 || index >= item->num_fields) ? NULL : item->keys[index]; }
int item_get_column_index(const Item *item, const char *col_name) {
    if (!item || !col_name) return -1;
    for (int i = 0; i < item->num_fields; i++) {
        if (strcmp(item->keys[i], col_name) == 0) return i;
    }
    return -1;
}
DataType item_get_value_type(const Item *item, int index) { return (!item || index < 0 || index >= item->num_fields) ? TYPE_NULL : item->values[index].type; }
bool item_get_value_bool(const Item *item, int index) { return (!item || index < 0 || index >= item->num_fields) ? false : item->values[index].data.b_val; }
int item_get_value_int(const Item *item, int index) { return (!item || index < 0 || index >= item->num_fields) ? 0 : item->values[index].data.i_val; }
float item_get_value_float(const Item *item, int index) { return (!item || index < 0 || index >= item->num_fields) ? 0.0f : item->values[index].data.f_val; }
bool item_get_value_as_double(const Item *item, int index, double *out_val) {
    if (!item || index < 0 || index >= item->num_fields || !out_val) return false;
    DataType type = item->values[index].type;
    if (type == TYPE_INT) { *out_val = (double)item->values[index].data.i_val; return true; }
    if (type == TYPE_FLOAT) { *out_val = (double)item->values[index].data.f_val; return true; }
    return false;
}
const char* item_get_value_string(const Item *item, int index) { return (!item || index < 0 || index >= item->num_fields) ? NULL : item->values[index].data.s_val; }