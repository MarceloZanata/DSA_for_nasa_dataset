
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lista_cruz.h"

#define MAX_VAL_STR 128

typedef struct no_cruzado_ {
    ITEM *item;                     /* Ponteiro único para o registro compartilhado */
    struct no_cruzado_ *prox_eixo1; /* Encadeamento na dimensão 1 */
    struct no_cruzado_ *prox_eixo2; /* Encadeamento na dimensão 2 */
} NoCruzado;

typedef struct cabeca_eixo_ {
    char valor[MAX_VAL_STR];
    NoCruzado *primeiro;
    NoCruzado *ultimo;
    struct cabeca_eixo_ *prox;
} CabecaEixo;

struct lista_cruz {
    CabecaEixo *eixo1;
    CabecaEixo *eixo2;
    char *campo_eixo1;
    char *campo_eixo2;
    int tamanho;
};

static void extrair_valor_campo(const ITEM *item, const char *campo, char *out_val, size_t max_len) {
    out_val[0] = '\0';
    if (!item || !campo) return;

    int idx = item_get_column_index(item, campo);
    if (idx == -1) {
        snprintf(out_val, max_len, "N/A");
        return;
    }

    DataType type = item_get_value_type(item, idx);
    if (type == TYPE_STRING) {
        const char *s = item_get_value_string(item, idx);
        snprintf(out_val, max_len, "%s", s ? s : "N/A");
    } else {
        double d = 0.0;
        if (item_get_value_as_double(item, idx, &d)) {
            snprintf(out_val, max_len, "%.0f", d);
        } else {
            snprintf(out_val, max_len, "N/A");
        }
    }
}

static CabecaEixo* obter_ou_criar_eixo(CabecaEixo **raiz, const char *valor) {
    CabecaEixo *atual = *raiz;
    CabecaEixo *ant = NULL;

    while (atual != NULL && strcmp(atual->valor, valor) != 0) {
        ant = atual;
        atual = atual->prox;
    }

    if (atual == NULL) {
        atual = (CabecaEixo*) malloc(sizeof(CabecaEixo));
        if (!atual) return NULL;
        strncpy(atual->valor, valor, MAX_VAL_STR - 1);
        atual->valor[MAX_VAL_STR - 1] = '\0';
        atual->primeiro = NULL;
        atual->ultimo = NULL;
        atual->prox = NULL;

        if (ant == NULL) *raiz = atual;
        else ant->prox = atual;
    }

    return atual;
}

ListaCruz* lista_cruz_criar(const char *campo_eixo1, const char *campo_eixo2) {
    ListaCruz *lista = (ListaCruz*) malloc(sizeof(ListaCruz));
    if (lista != NULL) {
        lista->eixo1 = NULL;
        lista->eixo2 = NULL;
        lista->campo_eixo1 = campo_eixo1 ? strdup(campo_eixo1) : strdup("threat_category");
        lista->campo_eixo2 = campo_eixo2 ? strdup(campo_eixo2) : strdup("year");
        lista->tamanho = 0;
    }
    return lista;
}

void lista_cruz_apagar(ListaCruz **lista) {
    if (lista == NULL || *lista == NULL) return;

    /* Para evitar dupla liberação de itens, liberamos os nós e itens pelo eixo 1 */
    CabecaEixo *e1 = (*lista)->eixo1;
    while (e1 != NULL) {
        NoCruzado *n = e1->primeiro;
        while (n != NULL) {
            NoCruzado *temp = n;
            n = n->prox_eixo1;
            if (temp->item != NULL) {
                item_free(temp->item);
            }
            free(temp);
        }
        CabecaEixo *temp_e1 = e1;
        e1 = e1->prox;
        free(temp_e1);
    }

    /* No eixo 2 apenas liberamos os nós de cabeçalho de eixo */
    CabecaEixo *e2 = (*lista)->eixo2;
    while (e2 != NULL) {
        CabecaEixo *temp_e2 = e2;
        e2 = e2->prox;
        free(temp_e2);
    }

    free((*lista)->campo_eixo1);
    free((*lista)->campo_eixo2);
    free(*lista);
    *lista = NULL;
}

bool lista_cruz_inserir(ListaCruz *lista, ITEM *item) {
    if (lista == NULL || item == NULL) return false;

    char v1[MAX_VAL_STR], v2[MAX_VAL_STR];
    extrair_valor_campo(item, lista->campo_eixo1, v1, sizeof(v1));
    extrair_valor_campo(item, lista->campo_eixo2, v2, sizeof(v2));

    CabecaEixo *ce1 = obter_ou_criar_eixo(&lista->eixo1, v1);
    CabecaEixo *ce2 = obter_ou_criar_eixo(&lista->eixo2, v2);

    if (!ce1 || !ce2) return false;

    /* Um único nó alocado participa de ambas as dimensões simultaneamente */
    NoCruzado *novo = (NoCruzado*) malloc(sizeof(NoCruzado));
    if (!novo) return false;

    novo->item = item;
    novo->prox_eixo1 = NULL;
    novo->prox_eixo2 = NULL;

    /* Insere no fim da dimensão 1 para manter ordem de percurso */
    if (ce1->primeiro == NULL) {
        ce1->primeiro = novo;
        ce1->ultimo = novo;
    } else {
        ce1->ultimo->prox_eixo1 = novo;
        ce1->ultimo = novo;
    }

    /* Insere no fim da dimensão 2 */
    if (ce2->primeiro == NULL) {
        ce2->primeiro = novo;
        ce2->ultimo = novo;
    } else {
        ce2->ultimo->prox_eixo2 = novo;
        ce2->ultimo = novo;
    }

    lista->tamanho++;
    return true;
}

int lista_cruz_tamanho(const ListaCruz *lista) {
    return (lista != NULL) ? lista->tamanho : 0;
}

bool lista_cruz_vazia(const ListaCruz *lista) {
    return (lista == NULL || lista->tamanho == 0);
}

void lista_cruz_percorrer_eixo1(const ListaCruz *lista, void (*callback)(ITEM *item, void *dados_extras), void *dados_extras) {
    if (lista == NULL || callback == NULL) return;

    for (CabecaEixo *e1 = lista->eixo1; e1 != NULL; e1 = e1->prox) {
        for (NoCruzado *n = e1->primeiro; n != NULL; n = n->prox_eixo1) {
            callback(n->item, dados_extras);
        }
    }
}

void lista_cruz_percorrer_eixo2(const ListaCruz *lista, void (*callback)(ITEM *item, void *dados_extras), void *dados_extras) {
    if (lista == NULL || callback == NULL) return;

    for (CabecaEixo *e2 = lista->eixo2; e2 != NULL; e2 = e2->prox) {
        for (NoCruzado *n = e2->primeiro; n != NULL; n = n->prox_eixo2) {
            callback(n->item, dados_extras);
        }
    }
}
