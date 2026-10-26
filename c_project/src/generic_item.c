#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <stdbool.h>
#include "generic_item.h"

#define INITIAL_CAPACITY 1024
#define LINE_CHUNK 256

// Estrutura do valor do campo que será armazenado
struct value_ {
    DataType type;
    union {
        long long i_val;
        double f_val;
        bool b_val;
        char *s_val;
    } data;
};

// Estrutura do cabeçalho
struct csv_header_ {
    int num_cols;
    char **names;
    int refs;   //cabecalho compartilhado com os itens
};

// Estrutura do item genérico
struct generic_item_ {
    int num_fields;
    CSVHeader *header;  // Chaves vem daqui (compartilhado)
    Value *values;
    int refs;   // Contador de referencias para compartilhamento
    double similarity;  // Similaridade calculada na busca
};

// ------------------------------------------------------------------ /
// Auxiliares e verificadores de alocação                             /
// ------------------------------------------------------------------ /

// Faz o malloc da estrutura e verifica
static void *xmalloc(size_t n) {
    void *p = malloc(n ? n : 1);
    if (!p) { fprintf(stderr, "erro: sem memoria\n"); exit(EXIT_FAILURE); }
    return p;
}

// Faz realloc da estrutura e verifica
static void *xrealloc(void *ptr, size_t n) {
    void *p = realloc(ptr, n ? n : 1);
    if (!p) { fprintf(stderr, "erro: sem memoria\n"); exit(EXIT_FAILURE); }
    return p;
}

// Duplica string
static char *dup_str(const char *s) {
    size_t n = strlen(s) + 1;
    char *p = (char *) xmalloc(n);
    memcpy(p, s, n);
    return p;
}

// Verifica se é a mesma letra
static bool equals_ignore_case(const char *a, const char *b) {
    while (*a && *b) {
        if (tolower((unsigned char)*a) != tolower((unsigned char)*b)) return false;
        a++; b++;
    }
    return *a == '\0' && *b == '\0';
}

// Retorna o tipo de dado do item após a inferencia
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

// ------------------------------------------------------------------ /
// Parser de CSV, validação de frases que contenham virgulas          /
// ------------------------------------------------------------------ /

static int count_csv_fields(const char *line) {
    int count = 1;
    bool quoted = false;
    for (const char *p = line; *p; p++) {
        if (*p == '"') quoted = !quoted;
        else if (*p == ',' && !quoted) count++;
    }
    return count;
}

// Leitura e validação das linhas do csv
static int parse_csv_line(const char *line, char **out_fields, int max_fields) {
    int count = 0;
    const char *p = line;
    size_t cap = strlen(line) + 1;
    char *buf = (char *) xmalloc(cap);

    while (count < max_fields) {
        size_t n = 0, keep = 0;
        bool quoted = false, had_quote = false;

        while (*p == ' ' || *p == '\t') p++;

        for (;; p++) {
            char c = *p;
            if (quoted) {
                if (c == '\0') break;
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
        p++;
    }

    free(buf);

    int found = count;
    while (count < max_fields) out_fields[count++] = dup_str("");
    return found;
}

// ------------------------------------------------------------------ /
// Inferencia dos tipos de dados                                      /
// ------------------------------------------------------------------ /

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
        if (errno == ERANGE) return TYPE_FLOAT;
        return TYPE_INT;
    }
    return TYPE_FLOAT;
}

// Atualiza o tipo de item, caso necessário uma correção
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

// ------------------------------------------------------------ /
// Leitura de linhas                                            /
// -------------------------------------------------------------/

// Lê até \n de cada linha do CSV
char* csv_ler_linha(FILE *file) {
    if (!file) return NULL; // Valida o arquivo

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

    if (c == EOF && len == 0) {
        free(buf);
        return NULL;
    }

    while (len > 0 && buf[len - 1] == '\r') len--;
    buf[len] = '\0';
    return buf;
}

/* ------------------------------------------------------------------ */
/* Cabeçalho                                                           */
/* ------------------------------------------------------------------ */

// Lê a primeira linha do CSV (Cabeçalho)
CSVHeader* csv_header_ler(FILE *file) {
    char *line = csv_ler_linha(file);
    if (!line) return NULL;

    // Ajuste dos bytes iniciais dos arquivos
    const char *text = line;
    if ((unsigned char)text[0] == 0xEF && (unsigned char)text[1] == 0xBB &&
        (unsigned char)text[2] == 0xBF) {
        text += 3;
    }

    // Leitura efetiva do cabeçalho
    CSVHeader *h = (CSVHeader *) xmalloc(sizeof(CSVHeader));
    h->num_cols = count_csv_fields(text);
    h->names = (char **) xmalloc((size_t) h->num_cols * sizeof(char *));
    h->refs = 1;
    parse_csv_line(text, h->names, h->num_cols);

    free(line);
    return h;
}

// Destrutor do cabeçalho
void csv_header_free(CSVHeader *header) {
    if (!header) return;
    if (--header->refs > 0) return;
    for (int i = 0; i < header->num_cols; i++) free(header->names[i]);
    free(header->names);
    free(header);
}

// Retorna o número de colunas do CSV
int csv_header_get_num_cols(const CSVHeader *header) {
    return header ? header->num_cols : 0;
}

// Retorna o nome da coluna do CSV
const char* csv_header_get_col_name(const CSVHeader *header, int idx) {
    if (!header || idx < 0 || idx >= header->num_cols) return NULL;
    return header->names[idx];
}

// ------------------------------------------------------------ /
// Funções do item genérico                                     /
// ------------------------------------------------------------ /

// Gera um item a partir de uma linha do CSV
ITEM* item_criar_de_linha(const char *linha, const CSVHeader *header) {
    if (!linha || !header || header->num_cols <= 0) return NULL; // Valida os parâmetros da função

    // Remove os espaços das linhas
    const char *p = linha;
    while (isspace((unsigned char)*p)) p++;
    if (*p == '\0') return NULL;    // Se resultar em apenas um caracter NULO, não tinha nada de util

    // Pega a linha lida, divide ela nos campos através do parse
    int n = header->num_cols;
    char **fields = (char **) xmalloc((size_t) n * sizeof(char *));
    parse_csv_line(linha, fields, n);

    // Aloca o item para povoar seus campos
    ITEM *item = (ITEM *) xmalloc(sizeof(ITEM));
    item->num_fields = n;
    item->header = (CSVHeader *) header;    // Utiliza apenas um cabeçalho alocado em memória
    item->header->refs++;   // Evita memory leak
    item->values = (Value *) xmalloc((size_t) n * sizeof(Value));
    item->refs = 1;
    item->similarity = 0.0;

    // Infere os tipos de dados de cada campo do item
    for (int i = 0; i < n; i++) {
        DataType t = infer_single_type(fields[i]);
        item->values[i] = parse_value_with_type(fields[i], t);
        free(fields[i]);
    }
    free(fields);
    return item;
}

// Utiliza o mesmo item em memória em mais de um lugar, evita memory leak
ITEM* item_retain(ITEM *item) {
    if (item) {
        item->refs++;
    }
    return item;
}

// Seta o valor de similaridade de um item
void item_set_similarity(ITEM *item, double sim) {
    if (item) item->similarity = sim;
}

// Retorna a similaridade de um item
double item_get_similarity(const ITEM *item) {
    return item ? item->similarity : 0.0;
}

// ------------------------------------------------------------------/
// Compatibilidade: leitura completa em vetor                        /
// ----------------------------------------------------------------- /

/* Utilizei no inicio do projeto quando tive a ideia de fazer um item genérico.
   Eu pegava todo o arquivo, jogava em memória e colocava em um vetor da
   estrutura de item genérico, testei a busca linear e funcionou +-, porém a
   ideia do trabalho era utilizar a busca na própria estrutura escolhida na base.
*/

ITEM** read_csv_to_items(const char *filename, int *out_num_rows) {
    if (out_num_rows) *out_num_rows = 0;

    FILE *file = fopen(filename, "r");
    if (!file) return NULL; // Valida o arquivo

    // Faz a leitura do cabeçalho
    CSVHeader *header = csv_header_ler(file);
    if (!header) {
        fclose(file);
        return NULL;
    }

    // Inicialmente tem memória para ler 1024 itens
    int capacity = INITIAL_CAPACITY;
    int num_rows = 0;
    ITEM **items = (ITEM **) xmalloc((size_t) capacity * sizeof(ITEM *));

    // Lê linha por linha e carrega no vetor
    char *line;
    while ((line = csv_ler_linha(file)) != NULL) {
        ITEM *item = item_criar_de_linha(line, header);
        free(line);
        if (!item) continue;

        // Se estourar o buffer, aumenta a capacidade
        if (num_rows >= capacity) {
            capacity *= 2;
            items = (ITEM **) xrealloc(items, (size_t) capacity * sizeof(ITEM *));
        }
        items[num_rows++] = item;
    }

    fclose(file);
    csv_header_free(header);

    if (out_num_rows) *out_num_rows = num_rows;
    return items;
}

// ------------------------------------------------------------------ /
// Printa os itens e destroi                                              /
// ------------------------------------------------------------------ /

/* Printa item, baseado no print do PANDAS (a lib do python) quando imprime o dataframe com os tipos de itens.
   Utilizei bastante enquanto estava desenvolvendo a inferência de tipos, durante a implementação
   me deparei com o problema de inferir um tipo de dado único por coluna, (o PANDAS faz isso mesmo com campos nulos)
   porém, o programa perderia desempenho na leitura, pois o ideal é fazer uma leitura prévia, apenas para inferir tipos
   por ex: (10% do arquivo para inferir), após, ainda corre o risco de ter dados mal formatados.
   A solução foi: (Cada item tem o seu campo com inferencia própria), eliminando os possíveis problemas de acessos indevidos
   de campos Nulos.
*/
void item_print(const ITEM *item) {
    if (!item) return;  // Valida o item
    printf("{\n");

    // Printa ({
    //          coluna: valor, tipo de dado
    //          }
    // Semelhante ao pandas
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

// Destrutor do item
void item_free(ITEM *item) {
    if (!item) return;
    if (--item->refs > 0) return; // Ainda referenciado por outra estrutura, retorna para não dar vazamento

    // Libera a memória
    for (int i = 0; i < item->num_fields; i++) {
        if (item->values[i].type == TYPE_STRING) {
            free(item->values[i].data.s_val);
        }
    }
    free(item->values);
    csv_header_free(item->header);
    free(item);
}

// Destrutros do item que fica noa interface
void item_apagar(ITEM **item) {
    if (item && *item) {
        item_free(*item);
        *item = NULL;
    }
}

// Retrona da id do item (se o CSV for gente boa e for o a primeira coluna de int)
int item_get_chave(const ITEM *item) {
    if (!item) return 0;
    for (int i = 0; i < item->num_fields; i++) {
        if (item->values[i].type == TYPE_INT) {
            return (int)item->values[i].data.i_val;
        }
    }
    return 1;
}

// -----------------------------------------------------/
// Getters                                              /
// -----------------------------------------------------/

// Verifica se o index da coluna é valido
static bool valid_index(const ITEM *item, int index) {
    return item && index >= 0 && index < item->num_fields;
}

// Retorna o número de campos de um item (colunas)
int item_get_num_fields(const ITEM *item) { return item ? item->num_fields : 0; }

// Pega o nome do campo do cabeçalho do index especifico (nome da coluna especifica)
const char* item_get_key(const ITEM *item, int index) {
    return valid_index(item, index) ? item->header->names[index] : NULL;
}

// Busca o index com base no nome da coluuna
int item_get_column_index(const ITEM *item, const char *col_name) {
    if (!item || !col_name) return -1;
    for (int i = 0; i < item->num_fields; i++) {
        if (strcmp(item->header->names[i], col_name) == 0) return i;
    }
    return -1;
}

// Retorna o tipo de dado do campo do item
DataType item_get_value_type(const ITEM *item, int index) {
    return valid_index(item, index) ? item->values[index].type : TYPE_NULL;
}

// Retorna o valor do booleano do index em especifico
bool item_get_value_bool(const ITEM *item, int index) {
    if (!valid_index(item, index) || item->values[index].type != TYPE_BOOL) return false;
    return item->values[index].data.b_val;
}

// Retorna o valor do int do index em específico
long long item_get_value_int(const ITEM *item, int index) {
    if (!valid_index(item, index) || item->values[index].type != TYPE_INT) return 0;
    return item->values[index].data.i_val;
}

// Retorna o valor do float do index em específico
double item_get_value_float(const ITEM *item, int index) {
    if (!valid_index(item, index) || item->values[index].type != TYPE_FLOAT) return 0.0;
    return item->values[index].data.f_val;
}

// Retorna o float convertido para double do index em específico
bool item_get_value_as_double(const ITEM *item, int index, double *out_val) {
    if (!valid_index(item, index) || !out_val) return false;
    DataType type = item->values[index].type;
    if (type == TYPE_INT)   { *out_val = (double)item->values[index].data.i_val; return true; }
    if (type == TYPE_FLOAT) { *out_val = item->values[index].data.f_val; return true; }
    return false;
}

// Retorna a string do index em específico
const char* item_get_value_string(const ITEM *item, int index) {
    if (!valid_index(item, index) || item->values[index].type != TYPE_STRING) return NULL;
    return item->values[index].data.s_val;
}

// ----------------------------------------------/
// Escrita no         CSV                        /
// ----------------------------------------------/

// Escreve o cabeçalho no arquivo de saida
void csv_header_escrever(FILE *file, const CSVHeader *header) {
    if (!file || !header) return;       // Valida o arquivo e o cabeçalho

    for (int i = 0; i < header->num_cols; i++) {
        fprintf(file, "%s", header->names[i]);
        if (i < header->num_cols - 1) {
            fputc(',', file);
        }
    }
    fputc('\n', file);
}

// Escreve um item no CSV
void item_escrever_csv(FILE *file, const ITEM *item) {
    if (!file || !item) return; // Valida o item e o arquivo

    // Escreve em disco de acordo com o tipo de dado, para ter um melhor aproveitamento de disco
    // não faz sentido gravar um float ou um int como (char*)
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
            case TYPE_STRING: {
                const char *s = v->data.s_val ? v->data.s_val : "";
                if (strchr(s, ',') || strchr(s, '"') || strchr(s, '\n')) {
                    fputc('"', file);
                    for (const char *cp = s; *cp; cp++) {
                        if (*cp == '"') fputc('"', file);
                        fputc(*cp, file);
                    }
                    fputc('"', file);
                } else {
                    fprintf(file, "%s", s);
                }
                break;
            }
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
