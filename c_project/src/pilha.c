#include <stdio.h>
#include <stdlib.h>
#include "pilha.h"

typedef struct no NO;

// Nó da pilha 
struct no{
    ITEM *item;
    NO *anterior;
};

// Estrutura base da pilha
struct pilha {
    NO *topo;
    int tamanho;
};

// Construtor
PILHA *pilha_criar(void) {
    PILHA *pilha = (PILHA *) malloc(sizeof(PILHA));

    // Inicia uma pilha vazia
    if (pilha != NULL) {
        pilha->topo = NULL;
        pilha->tamanho = 0;
    }
    return pilha;
}

// Destrutor
void pilha_apagar(PILHA **pilha) {
    NO *atual;

    if (pilha == NULL || *pilha == NULL)  // Verifica se a pilha é valida
        return;

    // Percorre a pilha liberando memória, do topo até a base
    atual = (*pilha)->topo;
    while (atual != NULL) {
        NO *anterior = atual->anterior;
        item_free(atual->item);
        free(atual);
        atual = anterior;
    }

    // Desaloca a pilha
    free(*pilha);
    *pilha = NULL;
}

// Retorna se a pilha é vazia
bool pilha_vazia(PILHA *pilha) {
    return pilha == NULL || pilha->topo == NULL;
}

// Retrona se a pilha é cheia
bool pilha_cheia(PILHA *pilha) {
    // Como a pilha é dinâmica, nunca estará cheia
    (void) pilha;
    return false;
}

// Retorna o tamanho da pilha
int pilha_tamanho(PILHA *pilha) {
    return pilha != NULL ? pilha->tamanho : 0;
}

// Retorna o item do topo da pilha
ITEM *pilha_topo(PILHA *pilha) {
    if (pilha == NULL || pilha_vazia(pilha)) {
        return NULL;
    }
    return pilha->topo->item;
}

// Empilha (push)
bool pilha_empilhar(PILHA *pilha, ITEM *item) {
    NO *novo;

    if (pilha == NULL || item == NULL) // Verifica se a pilha e o item são validos
        return false;

    novo = (NO *) malloc(sizeof(NO));
    if (novo == NULL)
        return false;

    // Coloca o novo item no topo da pilha
    novo->item = item;
    novo->anterior = pilha->topo;
    pilha->topo = novo;
    pilha->tamanho++;
    return true;
}

// Desempilha (pop)
ITEM *pilha_desempilhar(PILHA *pilha) {
    NO *removido;
    ITEM *item;

    if (pilha == NULL || pilha_vazia(pilha))    // Verifica se a pilha é valida
        return NULL;

    // Remove o item do topo
    removido = pilha->topo;
    item = removido->item;
    pilha->topo = removido->anterior;
    pilha->tamanho--;
    free(removido);
    return item;    // Retorna o item que foi removido
}

// Imprime a pilha
void pilha_imprimir(PILHA *pilha) {
    NO *atual;

    if (pilha == NULL)  // Valida a pilha
        return;

    printf("Topo -> ");
    atual = pilha->topo;

    // Percorre a pilha até a base printando ela
    while (atual != NULL) {
        printf("%d ", item_get_chave(atual->item));
        atual = atual->anterior;
    }
    printf("<- Base\n");
}

// Inverte a pilha
void pilha_inverter(PILHA *pilha) {
    NO *atual;
    NO *anterior = NULL;

    if (pilha == NULL)  // Valida a pilha
        return;

    atual = pilha->topo;

    // Inverte a pilha, um ponteiro desce e o outro sobe
    while (atual != NULL) {
        NO *proximo = atual->anterior;
        atual->anterior = anterior;
        anterior = atual;
        atual = proximo;
    }

    pilha->topo = anterior;
}

// Percorre a pilha
void pilha_percorrer(const PILHA *pilha, void (*callback)(ITEM *item, void *dados_extras), void *dados_extras) {
    if (pilha == NULL || callback == NULL)
        return;

    NO *atual = pilha->topo;

    // Percorre a pilha até a base com uma função callback, para printar ou escrever em arquivo por ex
    while (atual != NULL) {
        callback(atual->item, dados_extras);
        atual = atual->anterior;
    }
}
