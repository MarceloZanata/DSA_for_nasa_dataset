#ifndef LISTA_ENC_H
#define LISTA_ENC_H

#include <stdbool.h>
#include "generic_item.h"

typedef struct lista_enc ListaEnc;

ListaEnc* lista_enc_criar(void);
void lista_enc_apagar(ListaEnc **lista);
bool lista_enc_inserir(ListaEnc *lista, ITEM *item);
ITEM* lista_enc_remover_fim(ListaEnc *lista);
int lista_enc_tamanho(const ListaEnc *lista);
bool lista_enc_vazia(const ListaEnc *lista);
void lista_enc_percorrer(const ListaEnc *lista, void (*callback)(ITEM *item, void *dados_extras), void *dados_extras);

#endif
