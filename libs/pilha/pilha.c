#include "pilha.h"

#include <stdio.h>
#include <stdlib.h>

typedef struct no NO;

struct no {
    ITEM *item;
    NO *anterior;
};

struct pilha {
    NO *topo;
    int tamanho;
};

PILHA *pilha_criar(void) {
    PILHA *pilha = malloc(sizeof(PILHA));

    if (pilha != NULL) {
        pilha->topo = NULL;
        pilha->tamanho = 0;
    }

    return pilha;
}

void pilha_apagar(PILHA **pilha) {
    NO *atual;

    if (pilha == NULL || *pilha == NULL) {
        return;
    }

    atual = (*pilha)->topo;
    while (atual != NULL) {
        NO *anterior = atual->anterior;
        item_apagar(&atual->item);
        free(atual);
        atual = anterior;
    }

    free(*pilha);
    *pilha = NULL;
}

bool pilha_vazia(PILHA *pilha) {
    return pilha != NULL && pilha->topo == NULL;
}

bool pilha_cheia(PILHA *pilha) {
    (void) pilha;
    return false;
}

int pilha_tamanho(PILHA *pilha) {
    return pilha != NULL ? pilha->tamanho : -1;
}

ITEM *pilha_topo(PILHA *pilha) {
    if (pilha == NULL || pilha_vazia(pilha)) {
        return NULL;
    }

    return pilha->topo->item;
}

bool pilha_empilhar(PILHA *pilha, ITEM *item) {
    NO *novo;

    if (pilha == NULL || item == NULL) {
        return false;
    }

    novo = malloc(sizeof(NO));
    if (novo == NULL) {
        return false;
    }

    novo->item = item;
    novo->anterior = pilha->topo;
    pilha->topo = novo;
    pilha->tamanho++;
    return true;
}

ITEM *pilha_desempilhar(PILHA *pilha) {
    NO *removido;
    ITEM *item;

    if (pilha == NULL || pilha_vazia(pilha)) {
        return NULL;
    }

    removido = pilha->topo;
    item = removido->item;
    pilha->topo = removido->anterior;
    pilha->tamanho--;
    free(removido);
    return item;
}

void pilha_imprimir(PILHA *pilha) {
    NO *atual;

    if (pilha == NULL) {
        return;
    }

    printf("Topo -> ");
    atual = pilha->topo;
    while (atual != NULL) {
        printf("%d ", item_get_chave(atual->item));
        atual = atual->anterior;
    }
    printf("<- Base\n");
}

void pilha_inverter(PILHA *pilha) {
    NO *atual;
    NO *anterior = NULL;

    if (pilha == NULL) {
        return;
    }

    atual = pilha->topo;
    while (atual != NULL) {
        NO *proximo = atual->anterior;
        atual->anterior = anterior;
        anterior = atual;
        atual = proximo;
    }

    pilha->topo = anterior;
}
