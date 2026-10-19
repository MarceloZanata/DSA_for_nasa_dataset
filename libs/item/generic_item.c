#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <stdbool.h>
#include "generic_item.h"

#define INITIAL_CAPACITY 1024
#define LINE_CHUNK 256

struct value_ {
    DataType type;
    union {
        long long i_val;
        double f_val;
        bool b_val;
        char *s_val;
    } data;
};

struct csv_header_ {
    int num_cols;
    char **names;
    int refs;               /* cabecalho compartilhado com os ITEMs */
};

struct generic_item_ {
    int num_fields;
    CSVHeader *header;      /* chaves vem daqui (compartilhado) */
    Value *values;
};

/* ------------------------------------------------------------------ */
/* Utilitarios internos (sem depender de extensoes POSIX)              */
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
 * Separa a linha em ate max_fields campos. Cada campo e alocado, sem as
 * aspas externas, com "" convertido em " e sem espacos nas pontas (espacos
 * dentro de aspas sao preservados). Campos faltantes viram "".
 * Retorna quantos campos existiam de fato na linha.
 */
static int parse_csv_line(const char *line, char **out_fields, int max_fields) {
    int count = 0;
    const char *p = line;
    size_t cap = strlen(line) + 1;
    char *buf = (char *) xmalloc(cap);       /* buffer de trabalho unico */

    while (count < max_fields) {
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
        out_fields[count++] = dup_str(buf);

        if (*p == '\0') break;
        p++;                                               /* pula a virgula */
    }

    free(buf);

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
        (void) strtoll(str, NULL, 10);
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
        case TYPE_NULL:
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
/* Leitura de linhas                                                   */
/* ------------------------------------------------------------------ */

char* csv_ler_linha(FILE *file) {
    if (!file) return NULL;

    size_t cap = LINE_CHUNK, len = 0;
    char *buf = (char *) xmalloc(cap);
    int c;

    while ((c = fgetc(file)) != EOF && c != '\n') {
        if (len + 1 >= cap) {
            cap *= 2;
            buf = (char *) xrealloc(buf, cap);
        }
        buf[len++] = (char) c;
    }

    if (c == EOF && len == 0) {                /* nada lido: fim do arquivo */
        free(buf);
        return NULL;
    }

    while (len > 0 && buf[len - 1] == '\r') len--;
    buf[len] = '\0';
    return buf;
}

/* ------------------------------------------------------------------ */
/* Cabecalho                                                           */
/* ------------------------------------------------------------------ */

CSVHeader* csv_header_ler(FILE *file) {
    char *line = csv_ler_linha(file);
    if (!line) return NULL;

    /* ignora BOM UTF-8 no inicio do arquivo */
    const char *text = line;
    if ((unsigned char)text[0] == 0xEF && (unsigned char)text[1] == 0xBB &&
        (unsigned char)text[2] == 0xBF) {
        text += 3;
    }

    CSVHeader *h = (CSVHeader *) xmalloc(sizeof(CSVHeader));
    h->num_cols = count_csv_fields(text);
    h->names = (char **) xmalloc((size_t) h->num_cols * sizeof(char *));
    h->refs = 1;
    parse_csv_line(text, h->names, h->num_cols);

    free(line);
    return h;
}

void csv_header_free(CSVHeader *header) {
    if (!header) return;
    if (--header->refs > 0) return;            /* ainda usado por algum ITEM */
    for (int i = 0; i < header->num_cols; i++) free(header->names[i]);
    free(header->names);
    free(header);
}

int csv_header_get_num_cols(const CSVHeader *header) {
    return header ? header->num_cols : 0;
}

const char* csv_header_get_col_name(const CSVHeader *header, int idx) {
    if (!header || idx < 0 || idx >= header->num_cols) return NULL;
    return header->names[idx];
}

/* ------------------------------------------------------------------ */
/* Criacao de ITEM                                                     */
/* ------------------------------------------------------------------ */

ITEM* item_criar_de_linha(const char *linha, const CSVHeader *header) {
    if (!linha || !header || header->num_cols <= 0) return NULL;

    const char *p = linha;
    while (isspace((unsigned char)*p)) p++;
    if (*p == '\0') return NULL;               /* linha em branco */

    int n = header->num_cols;
    char **fields = (char **) xmalloc((size_t) n * sizeof(char *));
    parse_csv_line(linha, fields, n);

    ITEM *item = (ITEM *) xmalloc(sizeof(ITEM));
    item->num_fields = n;
    item->header = (CSVHeader *) header;
    item->header->refs++;
    item->values = (Value *) xmalloc((size_t) n * sizeof(Value));

    for (int i = 0; i < n; i++) {
        DataType t = infer_single_type(fields[i]);
        item->values[i] = parse_value_with_type(fields[i], t);
        free(fields[i]);
    }
    free(fields);
    return item;
}

/* ------------------------------------------------------------------ */
/* Compatibilidade: leitura completa em vetor                          */
/* ------------------------------------------------------------------ */

ITEM** read_csv_to_items(const char *filename, int *out_num_rows) {
    if (out_num_rows) *out_num_rows = 0;

    FILE *file = fopen(filename, "r");
    if (!file) return NULL;

    CSVHeader *header = csv_header_ler(file);
    if (!header) {
        fclose(file);
        return NULL;
    }

    int capacity = INITIAL_CAPACITY;
    int num_rows = 0;
    ITEM **items = (ITEM **) xmalloc((size_t) capacity * sizeof(ITEM *));

    char *line;
    while ((line = csv_ler_linha(file)) != NULL) {
        ITEM *item = item_criar_de_linha(line, header);
        free(line);
        if (!item) continue;

        if (num_rows >= capacity) {
            capacity *= 2;
            items = (ITEM **) xrealloc(items, (size_t) capacity * sizeof(ITEM *));
        }
        items[num_rows++] = item;
    }

    fclose(file);
    csv_header_free(header);                   /* os itens mantem sua referencia */

    if (out_num_rows) *out_num_rows = num_rows;
    return items;
}

/* ------------------------------------------------------------------ */
/* Impressao e liberacao                                               */
/* ------------------------------------------------------------------ */

void item_print(const ITEM *item) {
    if (!item) return;
    printf("{\n");
    for (int i = 0; i < item->num_fields; i++) {
        const Value *v = &item->values[i];
        const char *key = item->header->names[i];

        printf("  \"%-20s\": ", key);

        if (v->type == TYPE_STRING) {
            const char *s = v->data.s_val ? v->data.s_val : "";
            int pad = 32 - (int)strlen(s) - 2;
            printf("\"%s\"%*s", s, pad > 0 ? pad : 0, "");
        } else {
            char buffer[64];
            switch (v->type) {
                case TYPE_BOOL:  snprintf(buffer, sizeof(buffer), "%s", v->data.b_val ? "true" : "false"); break;
                case TYPE_INT:   snprintf(buffer, sizeof(buffer), "%lld", v->data.i_val); break;
                case TYPE_FLOAT: snprintf(buffer, sizeof(buffer), "%.15g", v->data.f_val); break;
                default:         snprintf(buffer, sizeof(buffer), "null"); break;
            }
            printf("%-32s", buffer);
        }
        printf(" [%s]\n", dtype_to_string(v->type));
    }
    printf("}\n");
}

void item_free(ITEM *item) {
    if (!item) return;
    for (int i = 0; i < item->num_fields; i++) {
        if (item->values[i].type == TYPE_STRING) {
            free(item->values[i].data.s_val);
        }
    }
    free(item->values);
    csv_header_free(item->header);             /* solta a referencia */
    free(item);
}

/* ------------------------------------------------------------------ */
/* Getters                                                             */
/* ------------------------------------------------------------------ */

static bool valid_index(const ITEM *item, int index) {
    return item && index >= 0 && index < item->num_fields;
}

int item_get_num_fields(const ITEM *item) { return item ? item->num_fields : 0; }

const char* item_get_key(const ITEM *item, int index) {
    return valid_index(item, index) ? item->header->names[index] : NULL;
}

int item_get_column_index(const ITEM *item, const char *col_name) {
    if (!item || !col_name) return -1;
    for (int i = 0; i < item->num_fields; i++) {
        if (strcmp(item->header->names[i], col_name) == 0) return i;
    }
    return -1;
}

DataType item_get_value_type(const ITEM *item, int index) {
    return valid_index(item, index) ? item->values[index].type : TYPE_NULL;
}

bool item_get_value_bool(const ITEM *item, int index) {
    if (!valid_index(item, index) || item->values[index].type != TYPE_BOOL) return false;
    return item->values[index].data.b_val;
}

long long item_get_value_int(const ITEM *item, int index) {
    if (!valid_index(item, index) || item->values[index].type != TYPE_INT) return 0;
    return item->values[index].data.i_val;
}

double item_get_value_float(const ITEM *item, int index) {
    if (!valid_index(item, index) || item->values[index].type != TYPE_FLOAT) return 0.0;
    return item->values[index].data.f_val;
}

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

/* ------------------------------------------------------------------ */
/* Escrita em arquivo CSV                                              */
/* ------------------------------------------------------------------ */

void csv_header_escrever(FILE *file, const CSVHeader *header) {
    if (!file || !header) return;

    for (int i = 0; i < header->num_cols; i++) {
        fprintf(file, "%s", header->names[i]);
        if (i < header->num_cols - 1) {
            fputc(',', file);
        }
    }
    fputc('\n', file);
}

void item_escrever_csv(FILE *file, const ITEM *item) {
    if (!file || !item) return;

    for (int i = 0; i < item->num_fields; i++) {
        const Value *v = &item->values[i];
        switch (v->type) {
            case TYPE_BOOL:
                fprintf(file, "%s", v->data.b_val ? "true" : "false");
                break;
            case TYPE_INT:
                fprintf(file, "%lld", v->data.i_val);
                break;
            case TYPE_FLOAT:
                fprintf(file, "%.15g", v->data.f_val);
                break;
            case TYPE_STRING:
                fprintf(file, "%s", v->data.s_val ? v->data.s_val : "");
                break;
            case TYPE_NULL:
            default:
                break;
        }
        if (i < item->num_fields - 1) {
            fputc(',', file);
        }
    }
    fputc('\n', file);
}
