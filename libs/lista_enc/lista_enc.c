#include <stdio.h>
#include <stdlib.h>
#include "lista_enc.h"

typedef struct no_ NO;

struct no_ {
    ITEM *item;
    NO *prox;
};

struct lista_enc {
    NO *inicio;
    NO *fim;
    int tamanho;
};

ListaEnc* lista_enc_criar(void) {
    ListaEnc *lista = (ListaEnc*) malloc(sizeof(ListaEnc));
    if (lista != NULL) {
        lista->inicio = NULL;
        lista->fim = NULL;
        lista->tamanho = 0;
    }
    return lista;
}

void lista_enc_apagar(ListaEnc **lista) {
    if (lista == NULL || *lista == NULL) return;

    NO *atual = (*lista)->inicio;
    while (atual != NULL) {
        NO *temp = atual;
        atual = atual->prox;
        if (temp->item != NULL) {
            item_free(temp->item);
        }
        free(temp);
    }
    free(*lista);
    *lista = NULL;
}

bool lista_enc_inserir(ListaEnc *lista, ITEM *item) {
    if (lista == NULL || item == NULL) return false;

    NO *novo = (NO*) malloc(sizeof(NO));
    if (novo == NULL) return false;

    novo->item = item;
    novo->prox = NULL;

    if (lista->inicio == NULL) {
        lista->inicio = novo;
    } else {
        lista->fim->prox = novo;
    }
    lista->fim = novo;
    lista->tamanho++;
    return true;
}

ITEM* lista_enc_remover_fim(ListaEnc *lista) {
    if (lista == NULL || lista_enc_vazia(lista)) return NULL;

    NO *atual = lista->inicio;
    NO *ant = NULL;

    while (atual->prox != NULL) {
        ant = atual;
        atual = atual->prox;
    }

    ITEM *item = atual->item;
    if (ant == NULL) {
        lista->inicio = NULL;
        lista->fim = NULL;
    } else {
        ant->prox = NULL;
        lista->fim = ant;
    }

    free(atual);
    lista->tamanho--;
    return item;
}

int lista_enc_tamanho(const ListaEnc *lista) {
    return (lista != NULL) ? lista->tamanho : 0;
}

bool lista_enc_vazia(const ListaEnc *lista) {
    return (lista == NULL || lista->tamanho == 0);
}

void lista_enc_percorrer(const ListaEnc *lista, void (*callback)(ITEM *item, void *dados_extras), void *dados_extras) {
    if (lista == NULL || callback == NULL) return;

    NO *atual = lista->inicio;
    while (atual != NULL) {
        callback(atual->item, dados_extras);
        atual = atual->prox;
    }
}