#ifndef LISTA_SEQ_H
#define LISTA_SEQ_H

#include <stdbool.h>
#include "generic_item.h"

typedef struct lista_ ListaSeq;

ListaSeq* lista_seq_criar(void);
void lista_seq_apagar(ListaSeq **lista);
bool lista_seq_inserir(ListaSeq *lista, ITEM *item);
ITEM* lista_seq_remover_fim(ListaSeq *lista);
int lista_seq_tamanho(const ListaSeq *lista);
bool lista_seq_vazia(const ListaSeq *lista);
void lista_seq_percorrer(const ListaSeq *lista, void (*callback)(ITEM *item, void *dados_extras), void *dados_extras);

#endif /* LISTA_SEQ_H */
