
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include "lista_ord.h"

typedef struct no_ NO;

struct no_ {
    ITEM *item;
    NO *prox;
};

struct lista_ord {
    NO *inicio;
    char *campo_chave;
    bool decrescente;
    int tamanho;
};

static int comparar_itens(const ITEM *a, const ITEM *b, const char *campo) {
    if (!a || !b) return 0;

    if (campo != NULL && (strcasecmp(campo, "similarity") == 0 || strcmp(campo, "sim") == 0)) {
        double sim_a = item_get_similarity(a);
        double sim_b = item_get_similarity(b);
        if (sim_a < sim_b) return -1;
        if (sim_a > sim_b) return 1;
        return 0;
    }

    if (!campo || *campo == '\0') {
        int ca = item_get_chave(a);
        int cb = item_get_chave(b);
        if (ca < cb) return -1;
        if (ca > cb) return 1;
        return 0;
    }

    int idx_a = item_get_column_index(a, campo);
    int idx_b = item_get_column_index(b, campo);

    if (idx_a == -1 || idx_b == -1) return 0;

    DataType type_a = item_get_value_type(a, idx_a);
    double num_a = 0.0, num_b = 0.0;

    if (item_get_value_as_double(a, idx_a, &num_a) && item_get_value_as_double(b, idx_b, &num_b)) {
        if (num_a < num_b) return -1;
        if (num_a > num_b) return 1;
        return 0;
    }

    if (type_a == TYPE_STRING) {
        const char *sa = item_get_value_string(a, idx_a);
        const char *sb = item_get_value_string(b, idx_b);
        if (sa && sb) return strcmp(sa, sb);
    }

    return 0;
}

ListaOrd* lista_ord_criar(const char *campo_chave, bool decrescente) {
    ListaOrd *lista = (ListaOrd*) malloc(sizeof(ListaOrd));
    if (lista != NULL) {
        lista->inicio = NULL;
        lista->decrescente = decrescente;
        lista->tamanho = 0;
        if (campo_chave != NULL) {
            size_t len = strlen(campo_chave) + 1;
            lista->campo_chave = (char*) malloc(len);
            if (lista->campo_chave != NULL) {
                memcpy(lista->campo_chave, campo_chave, len);
            }
        } else {
            lista->campo_chave = NULL;
        }
    }
    return lista;
}

void lista_ord_apagar(ListaOrd **lista) {
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

    free((*lista)->campo_chave);
    free(*lista);
    *lista = NULL;
}

bool lista_ord_inserir(ListaOrd *lista, ITEM *item) {
    if (lista == NULL || item == NULL) return false;

    NO *novo = (NO*) malloc(sizeof(NO));
    if (novo == NULL) return false;

    novo->item = item;
    novo->prox = NULL;

    /* A ordem é estritamente mantida durante a inserção */
    NO *atual = lista->inicio;
    NO *ant = NULL;

    while (atual != NULL) {
        int cmp = comparar_itens(item, atual->item, lista->campo_chave);
        bool deve_inserir_antes = lista->decrescente ? (cmp > 0) : (cmp < 0);

        if (deve_inserir_antes) break;

        ant = atual;
        atual = atual->prox;
    }

    if (ant == NULL) {
        novo->prox = lista->inicio;
        lista->inicio = novo;
    } else {
        novo->prox = atual;
        ant->prox = novo;
    }

    lista->tamanho++;
    return true;
}

int lista_ord_tamanho(const ListaOrd *lista) {
    return (lista != NULL) ? lista->tamanho : 0;
}

bool lista_ord_vazia(const ListaOrd *lista) {
    return (lista == NULL || lista->tamanho == 0);
}

void lista_ord_percorrer(const ListaOrd *lista, void (*callback)(ITEM *item, void *dados_extras), void *dados_extras) {
    if (lista == NULL || callback == NULL) return;

    NO *atual = lista->inicio;
    while (atual != NULL) {
        callback(atual->item, dados_extras);
        atual = atual->prox;
    }
}
