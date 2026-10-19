#ifndef GENERIC_ITEM_H
#define GENERIC_ITEM_H

#include <stdio.h>
#include <stdbool.h>

typedef enum {
    TYPE_NULL,
    TYPE_INT,
    TYPE_FLOAT,
    TYPE_BOOL,
    TYPE_STRING
} DataType;

/* Tipos opacos para encapsulamento */
typedef struct value_ Value;
typedef struct generic_item_ ITEM;
typedef struct csv_header_ CSVHeader;

/* ------------------------------------------------------------------ */
/* Cabecalho do CSV                                                    */
/* ------------------------------------------------------------------ */

/*
 * Le a primeira linha do arquivo (ja aberto) e extrai os nomes das colunas.
 * Apos a chamada, o ponteiro do arquivo fica no inicio da segunda linha.
 * Retorna NULL se o arquivo for NULL ou estiver vazio.
 * O cabecalho e compartilhado (contagem de referencias) com os ITEMs criados
 * a partir dele; csv_header_free() pode ser chamado a qualquer momento.
 */
CSVHeader* csv_header_ler(FILE *file);
void csv_header_free(CSVHeader *header);
int csv_header_get_num_cols(const CSVHeader *header);
const char* csv_header_get_col_name(const CSVHeader *header, int idx);

/* ------------------------------------------------------------------ */
/* Leitura em fluxo                                                    */
/* ------------------------------------------------------------------ */

/*
 * Le uma linha completa (qualquer tamanho) do arquivo, sem '\r' e '\n'.
 * Retorna string alocada (liberar com free) ou NULL no fim do arquivo.
 */
char* csv_ler_linha(FILE *file);

/*
 * Cria um unico ITEM a partir de uma linha de dados.
 * Cada celula tem seu tipo inferido individualmente (int, float, bool,
 * string ou null para campo vazio). Campos faltantes viram null; campos
 * excedentes sao ignorados.
 * Retorna NULL se a linha for vazia/so espacos ou se os argumentos forem NULL.
 */
ITEM* item_criar_de_linha(const char *linha, const CSVHeader *header);

/*
 * Compatibilidade: le o arquivo inteiro para um vetor de ITEM*.
 * (Internamente usa csv_header_ler + csv_ler_linha + item_criar_de_linha.)
 * O vetor retornado deve ser liberado com free() apos item_free() de cada item.
 */
ITEM** read_csv_to_items(const char *filename, int *out_num_rows);

/* ------------------------------------------------------------------ */
/* Ciclo de vida e impressao                                           */
/* ------------------------------------------------------------------ */

void item_free(ITEM *item);
void item_print(const ITEM *item);
const char* dtype_to_string(DataType type);

/* Utilitarios de tipos */
DataType infer_single_type(const char *str);
DataType promote_type(DataType current_type, DataType new_type);
Value parse_value_with_type(const char *raw_text, DataType col_type);

/* ------------------------------------------------------------------ */
/* Getters                                                             */
/* ------------------------------------------------------------------ */

int item_get_num_fields(const ITEM *item);
const char* item_get_key(const ITEM *item, int index);
int item_get_column_index(const ITEM *item, const char *col_name);
DataType item_get_value_type(const ITEM *item, int index);

bool item_get_value_bool(const ITEM *item, int index);
long long item_get_value_int(const ITEM *item, int index);   /* int64   */
double item_get_value_float(const ITEM *item, int index);    /* float64 */
bool item_get_value_as_double(const ITEM *item, int index, double *out_val);
const char* item_get_value_string(const ITEM *item, int index);

/* ------------------------------------------------------------------ */
/* Escrita em arquivo CSV                                              */
/* ------------------------------------------------------------------ */

/* Escreve a linha do cabeçalho no arquivo de saída no formato CSV */
void csv_header_escrever(FILE *file, const CSVHeader *header);

/* Escreve um único item formatado em uma linha do arquivo CSV de saída */
void item_escrever_csv(FILE *file, const ITEM *item);

#endif /* GENERIC_ITEM_H */
