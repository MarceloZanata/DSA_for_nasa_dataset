#ifndef LISTA_GEN_H
#define LISTA_GEN_H

#include <stdbool.h>
#include "generic_item.h"

typedef struct lista_gen ListaGen;

ListaGen* lista_gen_criar(const char *campo_grupo1, const char *campo_grupo2);
void lista_gen_apagar(ListaGen **lista);
bool lista_gen_inserir(ListaGen *lista, ITEM *item);
int lista_gen_tamanho(const ListaGen *lista);
bool lista_gen_vazia(const ListaGen *lista);
void lista_gen_percorrer(const ListaGen *lista, void (*callback)(ITEM *item, void *dados_extras), void *dados_extras);

#endif /* LISTA_GEN_H */
