#ifndef LISTA_CRUZ_H
#define LISTA_CRUZ_H

#include <stdbool.h>
#include "generic_item.h"

typedef struct lista_cruz ListaCruz;

ListaCruz* lista_cruz_criar(const char *campo_eixo1, const char *campo_eixo2);
void lista_cruz_apagar(ListaCruz **lista);
bool lista_cruz_inserir(ListaCruz *lista, ITEM *item);
int lista_cruz_tamanho(const ListaCruz *lista);
bool lista_cruz_vazia(const ListaCruz *lista);
void lista_cruz_percorrer_eixo1(const ListaCruz *lista, void (*callback)(ITEM *item, void *dados_extras), void *dados_extras);
void lista_cruz_percorrer_eixo2(const ListaCruz *lista, void (*callback)(ITEM *item, void *dados_extras), void *dados_extras);

#endif
