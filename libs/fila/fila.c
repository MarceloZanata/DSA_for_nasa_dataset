#include <stdio.h>
#include <stdlib.h>
#include "fila.h"

typedef struct no_ NO;

struct no_ {
    ITEM *item;
    NO *prox;
};

struct fila_ {
    NO *inicio;
    NO *fim;
    int tamanho;
};

FILA *fila_criar(void) {
    FILA *fila = malloc(sizeof(FILA));
    if (fila != NULL) {
        fila->inicio = NULL;
        fila->fim = NULL;
        fila->tamanho = 0;
    }
    return fila;
}

bool fila_inserir(FILA *fila, ITEM *item) {
    NO *novo;

    if (fila == NULL || item == NULL)
        return false;

    novo = malloc(sizeof(NO));
    if (novo == NULL)
        return false;

    novo->item = item;
    novo->prox = NULL;

    if (fila_vazia(fila)) {
        fila->inicio = novo;
        fila->fim = novo;
    } else {
        fila->fim->prox = novo;
        fila->fim = novo;
    }

    fila->tamanho++;
    return true;
}

ITEM *fila_remover(FILA *fila) {
    NO *removido;
    ITEM *item;

    if (fila == NULL || fila_vazia(fila))
        return NULL;

    removido = fila->inicio;
    item = removido->item;
    fila->inicio = removido->prox;
    free(removido);
    fila->tamanho--;

    if (fila->tamanho == 0)
        fila->fim = NULL;

    return item;
}

void fila_apagar(FILA **fila) {
    ITEM *item;

    if (fila == NULL || *fila == NULL)
        return;

    while (!fila_vazia(*fila)) {
        item = fila_remover(*fila);
        item_apagar(&item);
    }

    free(*fila);
    *fila = NULL;
}

ITEM *fila_frente(FILA *fila) {
    if (fila == NULL || fila_vazia(fila))
        return NULL;
    return fila->inicio->item;
}

int fila_tamanho(FILA *fila) {
    return fila != NULL ? fila->tamanho : 0;
}

bool fila_vazia(FILA *fila) {
    return fila == NULL || fila->tamanho == 0;
}

bool fila_cheia(FILA *fila) {
    /* Em uma fila dinâmica não há capacidade fixa. A falha é detectada
       na tentativa real de alocar o novo nó em fila_inserir. */
    return fila == NULL;
}

void fila_imprimir(FILA *fila) {
    NO *atual;

    if (fila == NULL) {
        printf("Fila inexistente.\n");
        return;
    }

    printf("inicio -> ");
    atual = fila->inicio;
    while (atual != NULL) {
        printf("[%d] ", item_get_chave(atual->item));
        atual = atual->prox;
    }
    printf("<- fim\n");
}
