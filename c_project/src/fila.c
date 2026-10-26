#include <stdio.h>
#include <stdlib.h>
#include "fila.h"

typedef struct no_ NO;

// Nó da fila
struct no_ {
    ITEM *item;
    NO *prox;
};

// Estrutura base da fila, com ponteiro para inicio e fim
struct fila_ {
    NO *inicio;
    NO *fim;
    int tamanho;
};

// Construtor
FILA *fila_criar(void) {
    FILA *fila = (FILA *) malloc(sizeof(FILA));
    // Inicia uma fila vazia
    if (fila != NULL) {
        fila->inicio = NULL;
        fila->fim = NULL;
        fila->tamanho = 0;
    }
    return fila;
}

// Insere no fim
bool fila_inserir(FILA *fila, ITEM *item) {
    NO *novo;

    // Verifica se a fila e o item são validos
    if (fila == NULL || item == NULL)
        return false;

    novo = (NO *) malloc(sizeof(NO));
    if (novo == NULL)
        return false;

    novo->item = item;
    novo->prox = NULL;

    // Inserção
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

// Remove o primeiro item da fila (FIFO)
ITEM *fila_remover(FILA *fila) {
    NO *removido;
    ITEM *item;

    if (fila == NULL || fila_vazia(fila))   // Verifica se a fila é valida
        return NULL;

    // Armazena o ponteiro para o inicio, atualiza os ponteiro inicio
    // Libera o ponteiro do primeiro item antigo
    removido = fila->inicio;
    item = removido->item;
    fila->inicio = removido->prox;
    free(removido);
    fila->tamanho--;

    if (fila->tamanho == 0)
        fila->fim = NULL;

    return item;    // Retrona o item removido
}

// Destrutor
void fila_apagar(FILA **fila) {
    ITEM *item;

    if (fila == NULL || *fila == NULL) // Verifica se a fila é valida
        return;

    // Loop percorre a fila removendo até ser uma fila vazia
    while (!fila_vazia(*fila)) {
        item = fila_remover(*fila);
        item_free(item);
    }

    free(*fila);
    *fila = NULL;
}

// Retorna o item inicial
ITEM *fila_frente(FILA *fila) {
    if (fila == NULL || fila_vazia(fila))
        return NULL;
    return fila->inicio->item;
}

// Retorna o tamanho da fila
int fila_tamanho(FILA *fila) {
    return fila != NULL ? fila->tamanho : 0;
}

// Verifica se a fila é vazia
bool fila_vazia(FILA *fila) {
    return fila == NULL || fila->tamanho == 0;
}

// Verifica se a fila é cheia
bool fila_cheia(FILA *fila) {
    // Como ela é dinamica, nunca estará cheia 
    (void) fila;
    return false;
}

// Printa a fila
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

// Percorre a fila
void fila_percorrer(const FILA *fila, void (*callback)(ITEM *item, void *dados_extras), void *dados_extras) {
    if (fila == NULL || callback == NULL)   // Verifica se a fila e callback são validas
        return;

    // A cada item da fila chama a função callback
    NO *atual = fila->inicio;
    while (atual != NULL) {
        callback(atual->item, dados_extras);
        atual = atual->prox;
    }
}
