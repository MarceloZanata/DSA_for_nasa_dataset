#include <stdio.h>
#include <stdlib.h>
#include "lista_cabeca.h"

typedef struct no_ NO;

struct no_ {
    ITEM *item; /* NULL para o nó cabeça/sentinela real */
    NO *prox;
};

struct lista_cabeca {
    NO *cabeca; /* Sentinela real[cite: 1] */
    NO *fim;
    int tamanho;
};

ListaCabeca* lista_cabeca_criar(void) {
    ListaCabeca *lista = (ListaCabeca*) malloc(sizeof(ListaCabeca));
    if (lista == NULL) return NULL;

    NO *cabeca = (NO*) malloc(sizeof(NO));
    if (cabeca == NULL) {
        free(lista);
        return NULL;
    }

    cabeca->item = NULL; /* O nó cabeça nunca representa um registro[cite: 1] */
    cabeca->prox = NULL;

    lista->cabeca = cabeca;
    lista->fim = cabeca;
    lista->tamanho = 0;
    return lista;
}

void lista_cabeca_apagar(ListaCabeca **lista) {
    if (lista == NULL || *lista == NULL) return;

    NO *atual = (*lista)->cabeca;
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

bool lista_cabeca_inserir(ListaCabeca *lista, ITEM *item) {
    if (lista == NULL || item == NULL) return false;

    NO *novo = (NO*) malloc(sizeof(NO));
    if (novo == NULL) return false;

    novo->item = item;
    novo->prox = NULL;

    lista->fim->prox = novo;
    lista->fim = novo;
    lista->tamanho++;
    return true;
}

ITEM* lista_cabeca_remover(ListaCabeca *lista, int indice) {
    if (lista == NULL || lista_cabeca_vazia(lista) || indice < 0 || indice >= lista->tamanho) {
        return NULL;
    }

    NO *anterior = lista->cabeca;
    for (int i = 0; i < indice; i++) {
        anterior = anterior->prox;
    }

    NO *removido = anterior->prox;
    ITEM *item = removido->item;

    anterior->prox = removido->prox;
    if (removido == lista->fim) {
        lista->fim = anterior;
    }

    free(removido);
    lista->tamanho--;
    return item;
}

int lista_cabeca_tamanho(const ListaCabeca *lista) {
    return (lista != NULL) ? lista->tamanho : 0;
}

bool lista_cabeca_vazia(const ListaCabeca *lista) {
    return (lista == NULL || lista->tamanho == 0);
}

void lista_cabeca_percorrer(const ListaCabeca *lista, void (*callback)(ITEM *item, void *dados_extras), void *dados_extras) {
    if (lista == NULL || callback == NULL) return;

    /* Percurso inicia no primeiro nó útil (cabeca->prox)[cite: 1] */
    NO *atual = lista->cabeca->prox;
    while (atual != NULL) {
        callback(atual->item, dados_extras);
        atual = atual->prox;
    }
}