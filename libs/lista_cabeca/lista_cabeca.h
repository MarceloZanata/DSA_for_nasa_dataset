#ifndef LISTA_CABECA_H
#define LISTA_CABECA_H

#include <stdbool.h>
#include "generic_item.h"

typedef struct lista_cabeca ListaCabeca;

ListaCabeca* lista_cabeca_criar(void);
void lista_cabeca_apagar(ListaCabeca **lista);
bool lista_cabeca_inserir(ListaCabeca *lista, ITEM *item);
ITEM* lista_cabeca_remover(ListaCabeca *lista, int indice);
int lista_cabeca_tamanho(const ListaCabeca *lista);
bool lista_cabeca_vazia(const ListaCabeca *lista);
void lista_cabeca_percorrer(const ListaCabeca *lista, void (*callback)(ITEM *item, void *dados_extras), void *dados_extras);

#endif