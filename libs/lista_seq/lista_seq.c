#include <stdio.h>
#include <stdlib.h>
#include "lista_seq.h"

#define CAPACIDADE_INICIAL 10

struct lista_ {
    ITEM **itens;      // Ponteiro dinâmico para os itens
    int inicio;        // Mantido da sua estrutura original
    int fim;           // Índice da próxima inserção
    int tamanho;       // Quantidade atual de elementos
    int capacidade;    // Capacidade máxima atual do vetor
};

ListaSeq* lista_seq_criar(void) {
    ListaSeq *lista = (ListaSeq*) malloc(sizeof(ListaSeq));
    if (lista != NULL) {
        lista->capacidade = CAPACIDADE_INICIAL;
        lista->tamanho = 0;
        lista->inicio = 0;
        lista->fim = 0;
        lista->itens = (ITEM**) malloc(lista->capacidade * sizeof(ITEM*));
        if (lista->itens == NULL) {
            free(lista);
            return NULL;
        }
    }
    return lista;
}

void lista_seq_apagar(ListaSeq **lista) {
    if (lista == NULL || *lista == NULL)
        return;
    
    // Libera os itens armazenados (se necessário liberar individualmente)
    for (int i = 0; i < (*lista)->tamanho; i++) {
        item_free((*lista)->itens[i]); // Ajuste para a função de liberar item do seu projeto
    }
    
    free((*lista)->itens);
    free(*lista);
    *lista = NULL;
}

bool lista_seq_inserir(ListaSeq *lista, ITEM *item) {
    if (lista == NULL || item == NULL)
        return false;

    // Se o vetor estourar a capacidade, redimensiona automaticamente com realloc
    if (lista->tamanho >= lista->capacidade) {
        int nova_capacidade = lista->capacidade * 2;
        ITEM **temp = (ITEM**) realloc(lista->itens, nova_capacidade * sizeof(ITEM*));
        if (temp == NULL) {
            return false; // Falha de alocação de memória
        }
        lista->itens = temp;
        lista->capacidade = nova_capacidade;
    }

    lista->itens[lista->fim++] = item;
    lista->tamanho++;
    return true;
}

ITEM* lista_seq_remover_fim(ListaSeq *lista) {
    if (lista == NULL || lista_seq_vazia(lista))
        return NULL;
    
    lista->fim--;
    ITEM *item = lista->itens[lista->fim];
    lista->itens[lista->fim] = NULL;
    lista->tamanho--;
    return item;
}

int lista_seq_tamanho(const ListaSeq *lista) {
    return lista == NULL ? 0 : lista->tamanho;
}

bool lista_seq_vazia(const ListaSeq *lista) {
    return lista == NULL || lista->tamanho == 0;
}

// Função essencial para percorrer a base sem acoplar a lógica de busca dentro do TAD
void lista_seq_percorrer(const ListaSeq *lista, void (*callback)(ITEM *item, void *dados_extras), void *dados_extras) {
    if (lista == NULL || callback == NULL)
        return;

    for (int i = 0; i < lista->tamanho; i++) {
        callback(lista->itens[i], dados_extras);
    }
}