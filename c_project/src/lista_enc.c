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

// Construtor
ListaEnc* lista_enc_criar(void) {
    ListaEnc *lista = (ListaEnc*) malloc(sizeof(ListaEnc));
    if (lista != NULL) {
        lista->inicio = NULL;
        lista->fim = NULL;
        lista->tamanho = 0;
    }
    return lista;
}

// Destrutor
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

// Insere no fim da lista
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

// Remove do fim da lista
ITEM* lista_enc_remover_fim(ListaEnc *lista) {
    if (lista == NULL || lista_enc_vazia(lista)) return NULL;

    NO *atual = lista->inicio;
    NO *ant = NULL;

    // Atualiza o ponteiro do item que será removido no final
    while (atual->prox != NULL) {
        ant = atual;
        atual = atual->prox;
    }

    // Atualiza os ponteiros para desvincular o excluido
    ITEM *item = atual->item;
    if (ant == NULL) {
        lista->inicio = NULL;
        lista->fim = NULL;
    } else {
        ant->prox = NULL;
        lista->fim = ant;
    }

    // Libera a memória
    free(atual);
    lista->tamanho--;
    return item;
}

// Retorna o tamanho da lista
int lista_enc_tamanho(const ListaEnc *lista) {
    return (lista != NULL) ? lista->tamanho : 0;
}

// Verifica se a lista está vazia
bool lista_enc_vazia(const ListaEnc *lista) {
    return (lista == NULL || lista->tamanho == 0);
}

// Percorre a lista
void lista_enc_percorrer(const ListaEnc *lista, void (*callback)(ITEM *item, void *dados_extras), void *dados_extras) {
    if (lista == NULL || callback == NULL) return;

    NO *atual = lista->inicio;
    while (atual != NULL) {
        callback(atual->item, dados_extras);
        atual = atual->prox;
    }
}
