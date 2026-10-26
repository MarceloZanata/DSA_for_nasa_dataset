#include <stdio.h>
#include <stdlib.h>
#include "lista_cabeca.h"

typedef struct no_ NO;

struct no_ {
    ITEM *item; // NULL para o nó cabeça
    NO *prox;
};

struct lista_cabeca {
    NO *cabeca; // Sentinela real alocado
    NO *fim;
    int tamanho;
};

// Construtor da lista vazia
ListaCabeca* lista_cabeca_criar(void) {
    ListaCabeca *lista = (ListaCabeca*) malloc(sizeof(ListaCabeca));
    if (lista == NULL) return NULL;

    NO *cabeca = (NO*) malloc(sizeof(NO));
    if (cabeca == NULL) {
        free(lista);
        return NULL;
    }

    cabeca->item = NULL; // O nó cabeça nunca representa um registro
    cabeca->prox = NULL;

    lista->cabeca = cabeca;
    lista->fim = cabeca;
    lista->tamanho = 0;
    return lista;
}

// Destrutor da lista
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

// Insere item na lista
bool lista_cabeca_inserir(ListaCabeca *lista, ITEM *item) {
    if (lista == NULL || item == NULL) return false;

    NO *novo = (NO*) malloc(sizeof(NO));
    if (novo == NULL) return false;

    novo->item = item;
    novo->prox = NULL;

    // Insere no fim e atualiza o fim
    lista->fim->prox = novo;
    lista->fim = novo;
    lista->tamanho++;
    return true;
}

// Remove Item no indice especificado
ITEM* lista_cabeca_remover(ListaCabeca *lista, int indice) {
    if (lista == NULL || lista_cabeca_vazia(lista) || indice < 0 || indice >= lista->tamanho) {
        return NULL;
    }

    // Começa do inicio e percorre até o indice desejado
    NO *anterior = lista->cabeca;
    for (int i = 0; i < indice; i++) {
        anterior = anterior->prox;
    }

    // Guarda o ponteiro do item que será removido
    NO *removido = anterior->prox;
    ITEM *item = removido->item;

    // Atualiza os ponteiros do proximo e do anterior
    anterior->prox = removido->prox;
    if (removido == lista->fim) {
        lista->fim = anterior;
    }

    // Libera a memória que estava alocado o Item do indice x
    free(removido);
    lista->tamanho--;
    return item;
}

// Retorna o tamanho da lista
int lista_cabeca_tamanho(const ListaCabeca *lista) {
    return (lista != NULL) ? lista->tamanho : 0;
}

// Retorna se a lista é vazia
bool lista_cabeca_vazia(const ListaCabeca *lista) {
    return (lista == NULL || lista->tamanho == 0);
}

// Percorre a lista
void lista_cabeca_percorrer(const ListaCabeca *lista, void (*callback)(ITEM *item, void *dados_extras), void *dados_extras) {
    if (lista == NULL || callback == NULL) return;

    // Percorre a lista partindo da cabeça
    NO *atual = lista->cabeca->prox;
    while (atual != NULL) {
        callback(atual->item, dados_extras);
        atual = atual->prox;
    }
}
