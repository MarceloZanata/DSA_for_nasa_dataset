#ifndef LISTA_ORD_H
#define LISTA_ORD_H

#include <stdbool.h>
#include "generic_item.h"

typedef struct lista_ord ListaOrd;

ListaOrd* lista_ord_criar(const char *campo_chave, bool decrescente);
void lista_ord_apagar(ListaOrd **lista);
bool lista_ord_inserir(ListaOrd *lista, ITEM *item);
int lista_ord_tamanho(const ListaOrd *lista);
bool lista_ord_vazia(const ListaOrd *lista);
void lista_ord_percorrer(const ListaOrd *lista, void (*callback)(ITEM *item, void *dados_extras), void *dados_extras);

#endif