#ifndef ESTRUTURA_H
#define ESTRUTURA_H

#include <stdio.h>
#include <stdbool.h>
#include "generic_item.h"
#include "generic_search.h"

typedef enum {
    EST_PILHA,
    EST_FILA,
    EST_LISTA_SEQ,
    EST_LISTA_ENC,
    EST_LISTA_CABECA,
    EST_LISTA_ORD,
    EST_LISTA_GEN,
    EST_LISTA_CRUZ,
    EST_DESCONHECIDA
} TipoEstrutura;

typedef struct estrutura_t Estrutura;

Estrutura* estrutura_criar(const char *nome, const Query *q);
bool estrutura_inserir(Estrutura *est, ITEM *item);
int estrutura_tamanho(const Estrutura *est);
void estrutura_percorrer(const Estrutura *est, void (*callback)(ITEM *item, void *dados_extras), void *dados_extras);
int estrutura_exportar_top_k(Estrutura *est, FILE *arquivo_saida, int top_k);
void estrutura_apagar(Estrutura **est);
const char* estrutura_nome(const Estrutura *est);

#endif
