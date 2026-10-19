#ifndef GENERIC_SEARCH_H
#define GENERIC_SEARCH_H

#include <stddef.h>
#include <stdbool.h>
#include "generic_item.h"

typedef struct query_ Query;

/*
 * Abre e interpreta o arquivo de consulta (.txt), linha a linha.
 * Comandos (maiusculas/minusculas tanto faz; '|' e/ou espacos separam os
 * argumentos; linhas em branco e iniciadas por '#' sao ignoradas):
 *
 *   FILTER <campo>|<op>|<alvo>        op: ==  !=  <=  >=  <  >
 *   SIM    <campo>|<alvo>|<tolerancia>|<peso>
 *   ACCEPT <limiar>
 *   ORDER  <campo>|<ASC|DESC>         (campo pode ser "similarity")
 *   GROUP  <campo1>|<campo2>
 *   CROSS  <campo1>|<campo2>
 *
 * Nomes de campo ou alvos com espacos podem ser escritos entre aspas duplas.
 * Linhas invalidas geram um aviso em stderr e sao ignoradas.
 * Retorna NULL se o arquivo nao puder ser aberto.
 */
Query* parse_query_file(const char *filename);
void query_free(Query *q);

/*
 * Avalia um UNICO ITEM.
 * Retorna true se o item passa em todos os FILTER e, havendo SIM e ACCEPT,
 * atinge a similaridade minima.
 * Se out_similarity != NULL, recebe a similaridade total calculada:
 *   - 0.0 se o item for reprovado em algum FILTER (a similaridade nem e calculada);
 *   - 1.0 se a consulta nao tem nenhum SIM (todos os filtros satisfeitos);
 *   - a media ponderada dos SIM nos demais casos (inclusive quando reprovado por ACCEPT).
 */
bool query_avaliar_item(const ITEM *item, const Query *q, double *out_similarity);

/* ------------------------------------------------------------------ */
/* Configuracoes de organizacao lidas do arquivo de consulta           */
/* ------------------------------------------------------------------ */

/*
 * Chave do ORDER, ou NULL se a consulta nao tiver ORDER. O ponteiro pertence
 * a Query. out_desc (pode ser NULL) recebe true para DESC e false para ASC.
 */
const char* query_get_order_key(const Query *q, bool *out_desc);

/* true se a chave do ORDER for "similarity" (ordenar pelo score calculado). */
bool query_order_is_similarity(const Query *q);

/*
 * Copiam as duas chaves para os buffers (truncando em max_len-1 e sempre
 * terminando em '\0'). Se o comando nao existir (ou so tiver uma chave), o
 * buffer correspondente recebe "".
 */
void query_get_group_keys(const Query *q, char *out_k1, char *out_k2, size_t max_len);
void query_get_cross_keys(const Query *q, char *out_k1, char *out_k2, size_t max_len);

/* true se a consulta tem GROUP / CROSS com as duas chaves. */
bool query_has_group(const Query *q);
bool query_has_cross(const Query *q);

#endif /* GENERIC_SEARCH_H */
