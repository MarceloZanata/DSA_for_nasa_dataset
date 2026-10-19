#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <stdbool.h>
#include "generic_item.h"

#define MAX_LINE_LEN 8192
#define INITIAL_CAPACITY 1024

struct value_ {
    DataType type;
    union {
        long long i_val;
        double f_val;
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

/* ------------------------------------------------------------------ */
/* Utilitários internos (sem depender de extensões POSIX)              */
/* ------------------------------------------------------------------ */

static void *xmalloc(size_t n) {
    void *p = malloc(n ? n : 1);
    if (!p) { fprintf(stderr, "erro: sem memoria\n"); exit(EXIT_FAILURE); }
    return p;
}

static void *xrealloc(void *ptr, size_t n) {
    void *p = realloc(ptr, n ? n : 1);
    if (!p) { fprintf(stderr, "erro: sem memoria\n"); exit(EXIT_FAILURE); }
    return p;
}

static char *dup_str(const char *s) {
    size_t n = strlen(s) + 1;
    char *p = (char *) xmalloc(n);
    memcpy(p, s, n);
    return p;
}

static bool equals_ignore_case(const char *a, const char *b) {
    while (*a && *b) {
        if (tolower((unsigned char)*a) != tolower((unsigned char)*b)) return false;
        a++; b++;
    }
    return *a == '\0' && *b == '\0';
}

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

/* ------------------------------------------------------------------ */
/* Parser de CSV (RFC 4180: campos entre aspas podem conter virgulas)  */
/* ------------------------------------------------------------------ */

/* Conta os campos de uma linha respeitando aspas. */
static int count_csv_fields(const char *line) {
    int count = 1;
    bool quoted = false;
    for (const char *p = line; *p; p++) {
        if (*p == '"') quoted = !quoted;   /* "" escapado alterna duas vezes */
        else if (*p == ',' && !quoted) count++;
    }
    return count;
}

/*
 * Separa a linha em ate max_fields campos. Cada campo e alocado (strdup),
 * sem as aspas externas, com "" convertido em " e sem espacos nas pontas
 * (espacos dentro de aspas sao preservados). Campos faltantes viram "".
 * Retorna quantos campos existiam de fato na linha.
 */
static int parse_csv_line(const char *line, char **out_fields, int max_fields) {
    int count = 0;
    const char *p = line;
    size_t cap = strlen(line) + 1;

    while (count < max_fields) {
        char *buf = (char *) xmalloc(cap);
        size_t n = 0, keep = 0;
        bool quoted = false, had_quote = false;

        while (*p == ' ' || *p == '\t') p++;

        for (;; p++) {
            char c = *p;
            if (quoted) {
                if (c == '\0') break;                      /* aspa nao fechada */
                if (c == '"') {
                    if (p[1] == '"') { buf[n++] = '"'; p++; }
                    else { quoted = false; keep = n; }
                } else {
                    buf[n++] = c;
                }
            } else {
                if (c == ',' || c == '\0') break;
                if (c == '"' && n == 0 && !had_quote) { quoted = true; had_quote = true; }
                else buf[n++] = c;
            }
        }

        while (n > keep && isspace((unsigned char)buf[n - 1])) n--;
        buf[n] = '\0';
        out_fields[count++] = buf;

        if (*p == '\0') break;
        p++;                                               /* pula a virgula */
    }

    int found = count;
    while (count < max_fields) out_fields[count++] = dup_str("");
    return found;
}

/* ------------------------------------------------------------------ */
/* Inferencia de tipos                                                 */
/* ------------------------------------------------------------------ */

/*
 * Aceita: [sinal] digitos [. digitos] [e|E [sinal] digitos]
 * (inclui notacao cientifica, ex.: 8.95e-05). Rejeita inf/nan/hex.
 */
DataType infer_single_type(const char *str) {
    if (str == NULL || *str == '\0') return TYPE_NULL;

    if (equals_ignore_case(str, "true") || equals_ignore_case(str, "false")) return TYPE_BOOL;

    const char *p = str;
    bool is_float = false;
    int mantissa_digits = 0;

    if (*p == '-' || *p == '+') p++;
    while (isdigit((unsigned char)*p)) { p++; mantissa_digits++; }
    if (*p == '.') {
        is_float = true;
        p++;
        while (isdigit((unsigned char)*p)) { p++; mantissa_digits++; }
    }
    if (mantissa_digits == 0) return TYPE_STRING;

    if (*p == 'e' || *p == 'E') {
        is_float = true;
        p++;
        if (*p == '-' || *p == '+') p++;
        if (!isdigit((unsigned char)*p)) return TYPE_STRING;
        while (isdigit((unsigned char)*p)) p++;
    }
    if (*p != '\0') return TYPE_STRING;

    if (!is_float) {
        errno = 0;
        strtoll(str, NULL, 10);
        if (errno == ERANGE) return TYPE_FLOAT;            /* inteiro grande demais */
        return TYPE_INT;
    }
    return TYPE_FLOAT;
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
    val.data.s_val = NULL;

    if (raw_text == NULL || *raw_text == '\0') {
        val.type = TYPE_NULL;
        return val;
    }

    switch (col_type) {
        case TYPE_BOOL:
            val.data.b_val = equals_ignore_case(raw_text, "true") || strcmp(raw_text, "1") == 0;
            break;
        case TYPE_INT:
            val.data.i_val = strtoll(raw_text, NULL, 10);
            break;
        case TYPE_FLOAT:
            val.data.f_val = strtod(raw_text, NULL);
            break;
        case TYPE_STRING:
        default:
            val.type = TYPE_STRING;
            val.data.s_val = dup_str(raw_text);
            break;
    }
    return val;
}

/* ------------------------------------------------------------------ */
/* Impressao e liberacao                                               */
/* ------------------------------------------------------------------ */

void item_print(const ITEM *item) {
    if (!item) return;
    printf("{\n");
    for (int i = 0; i < item->num_fields; i++) {
        char buffer[512];
        const Value *v = &item->values[i];

        switch (v->type) {
            case TYPE_NULL:   snprintf(buffer, sizeof(buffer), "null"); break;
            case TYPE_BOOL:   snprintf(buffer, sizeof(buffer), "%s", v->data.b_val ? "true" : "false"); break;
            case TYPE_INT:    snprintf(buffer, sizeof(buffer), "%lld", v->data.i_val); break;
            case TYPE_FLOAT:  snprintf(buffer, sizeof(buffer), "%.15g", v->data.f_val); break;
            default:          snprintf(buffer, sizeof(buffer), "\"%s\"", v->data.s_val); break;
        }
        printf("  \"%-20s\": %-32s [%s]\n", item->keys[i], buffer, dtype_to_string(v->type));
    }
    printf("}\n");
}

void item_free(ITEM *item) {
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

/* ------------------------------------------------------------------ */
/* Leitura do arquivo                                                  */
/* ------------------------------------------------------------------ */

ITEM** read_csv_to_items(const char *filename, int *out_num_rows) {
    if (out_num_rows) *out_num_rows = 0;

    FILE *file = fopen(filename, "r");
    if (!file) return NULL;

    char line[MAX_LINE_LEN];
    if (!fgets(line, sizeof(line), file)) {
        fclose(file);
        return NULL;
    }

    line[strcspn(line, "\r\n")] = 0;

    /* ignora BOM UTF-8 no inicio do arquivo */
    char *header_line = line;
    if ((unsigned char)header_line[0] == 0xEF && (unsigned char)header_line[1] == 0xBB &&
        (unsigned char)header_line[2] == 0xBF) {
        header_line += 3;
    }

    int num_cols = count_csv_fields(header_line);
    char **headers = (char**) xmalloc(num_cols * sizeof(char*));
    parse_csv_line(header_line, headers, num_cols);

    int capacity = INITIAL_CAPACITY;
    int num_rows = 0;
    RawRow *raw_rows = (RawRow*) xmalloc(capacity * sizeof(RawRow));

    DataType *column_types = (DataType*) xmalloc(num_cols * sizeof(DataType));
    for (int i = 0; i < num_cols; i++) column_types[i] = TYPE_NULL;

    while (fgets(line, sizeof(line), file)) {
        line[strcspn(line, "\r\n")] = 0;
        if (line[0] == '\0') continue;

        if (num_rows >= capacity) {
            capacity *= 2;
            raw_rows = (RawRow*) xrealloc(raw_rows, capacity * sizeof(RawRow));
        }

        raw_rows[num_rows].raw_fields = (char**) xmalloc(num_cols * sizeof(char*));
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

    ITEM **items = (ITEM**) xmalloc(num_rows * sizeof(ITEM*));

    for (int r = 0; r < num_rows; r++) {
        ITEM *new_item = (ITEM*) xmalloc(sizeof(ITEM));
        new_item->num_fields = num_cols;
        new_item->keys = (char**) xmalloc(num_cols * sizeof(char*));
        new_item->values = (Value*) xmalloc(num_cols * sizeof(Value));

        for (int c = 0; c < num_cols; c++) {
            new_item->keys[c] = dup_str(headers[c]);
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

    if (out_num_rows) *out_num_rows = num_rows;
    return items;
}

/* ------------------------------------------------------------------ */
/* Getters                                                             */
/* ------------------------------------------------------------------ */

static bool valid_index(const ITEM *item, int index) {
    return item && index >= 0 && index < item->num_fields;
}

int item_get_num_fields(const ITEM *item) { return item ? item->num_fields : 0; }
const char* item_get_key(const ITEM *item, int index) { return valid_index(item, index) ? item->keys[index] : NULL; }
int item_get_column_index(const ITEM *item, const char *col_name) {
    if (!item || !col_name) return -1;
    for (int i = 0; i < item->num_fields; i++) {
        if (strcmp(item->keys[i], col_name) == 0) return i;
    }
    return -1;
}
DataType item_get_value_type(const ITEM *item, int index) { return valid_index(item, index) ? item->values[index].type : TYPE_NULL; }
bool item_get_value_bool(const ITEM *item, int index) { return valid_index(item, index) ? item->values[index].data.b_val : false; }
long long item_get_value_int(const ITEM *item, int index) { return valid_index(item, index) ? item->values[index].data.i_val : 0; }
double item_get_value_float(const ITEM *item, int index) { return valid_index(item, index) ? item->values[index].data.f_val : 0.0; }
bool item_get_value_as_double(const ITEM *item, int index, double *out_val) {
    if (!valid_index(item, index) || !out_val) return false;
    DataType type = item->values[index].type;
    if (type == TYPE_INT)   { *out_val = (double)item->values[index].data.i_val; return true; }
    if (type == TYPE_FLOAT) { *out_val = item->values[index].data.f_val; return true; }
    return false;
}
const char* item_get_value_string(const ITEM *item, int index) {
    if (!valid_index(item, index) || item->values[index].type != TYPE_STRING) return NULL;
    return item->values[index].data.s_val;
}
