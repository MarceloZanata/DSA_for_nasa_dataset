
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lista_gen.h"

#define MAX_VAL_STR 128

// Estrutura do nó
typedef struct no_item_ {
    ITEM *item;
    struct no_item_ *prox;
} NoItem;

// Subgrupo da lista generalizada
typedef struct subgrupo_ {
    char valor_subgrupo[MAX_VAL_STR];
    NoItem *itens;
    struct subgrupo_ *prox;
} Subgrupo;

// Grupo 
typedef struct grupo_ {
    char valor_grupo[MAX_VAL_STR];
    Subgrupo *subgrupos;
    struct grupo_ *prox;
} Grupo;

// Estrutura geral da lista generalizada
struct lista_gen {
    Grupo *raiz;
    char *campo_grupo1;
    char *campo_grupo2;
    int total_itens;
};

// PEga valor do campo
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

// Construtor
ListaGen* lista_gen_criar(const char *campo_grupo1, const char *campo_grupo2) {
    ListaGen *lista = (ListaGen*) malloc(sizeof(ListaGen));
    if (lista != NULL) {
        lista->raiz = NULL;
        lista->total_itens = 0;
        lista->campo_grupo1 = campo_grupo1 ? strdup(campo_grupo1) : strdup("x");
        lista->campo_grupo2 = campo_grupo2 ? strdup(campo_grupo2) : strdup("y");
    }
    return lista;
}

// Destrutor
void lista_gen_apagar(ListaGen **lista) {
    if (lista == NULL || *lista == NULL) return;

    Grupo *g = (*lista)->raiz;
    while (g != NULL) {
        Subgrupo *s = g->subgrupos;

        // Remove os itens dos subgrupos
        while (s != NULL) {
            NoItem *ni = s->itens;
            while (ni != NULL) {
                NoItem *t_ni = ni;
                ni = ni->prox;
                if (t_ni->item != NULL) {
                    item_free(t_ni->item);
                }
                free(t_ni);
            }
            Subgrupo *t_s = s;
            s = s->prox;
            free(t_s);
        }

        // Remove o grupo
        Grupo *t_g = g;
        g = g->prox;
        free(t_g);
    }

    // Libera a memoria da lista
    free((*lista)->campo_grupo1);
    free((*lista)->campo_grupo2);
    free(*lista);
    *lista = NULL;
}


bool lista_gen_inserir(ListaGen *lista, ITEM *item) {
    if (lista == NULL || item == NULL) return false;    // Verifica se a lista é valida

    char v1[MAX_VAL_STR], v2[MAX_VAL_STR];
    extrair_valor_campo(item, lista->campo_grupo1, v1, sizeof(v1));
    extrair_valor_campo(item, lista->campo_grupo2, v2, sizeof(v2));

    /* Grupo Nível 1 */
    Grupo *g = lista->raiz;
    Grupo *g_ant = NULL;
    while (g != NULL && strcmp(g->valor_grupo, v1) != 0) {
        g_ant = g;
        g = g->prox;
    }
    if (g == NULL) {
        g = (Grupo*) malloc(sizeof(Grupo));
        if (!g) return false;
        strncpy(g->valor_grupo, v1, MAX_VAL_STR - 1);
        g->valor_grupo[MAX_VAL_STR - 1] = '\0';
        g->subgrupos = NULL;
        g->prox = NULL;
        if (g_ant == NULL) lista->raiz = g;
        else g_ant->prox = g;
    }

    /* Subgrupo Nível 2 */
    Subgrupo *s = g->subgrupos;
    Subgrupo *s_ant = NULL;
    while (s != NULL && strcmp(s->valor_subgrupo, v2) != 0) {
        s_ant = s;
        s = s->prox;
    }
    if (s == NULL) {
        s = (Subgrupo*) malloc(sizeof(Subgrupo));
        if (!s) return false;
        strncpy(s->valor_subgrupo, v2, MAX_VAL_STR - 1);
        s->valor_subgrupo[MAX_VAL_STR - 1] = '\0';
        s->itens = NULL;
        s->prox = NULL;
        if (s_ant == NULL) g->subgrupos = s;
        else s_ant->prox = s;
    }

    /* Insere o item mantendo ordem de chegada */
    NoItem *novo = (NoItem*) malloc(sizeof(NoItem));
    if (!novo) return false;
    novo->item = item;
    novo->prox = NULL;

    if (s->itens == NULL) {
        s->itens = novo;
    } else {
        NoItem *cur = s->itens;
        while (cur->prox != NULL) cur = cur->prox;
        cur->prox = novo;
    }

    lista->total_itens++;
    return true;
}

int lista_gen_tamanho(const ListaGen *lista) {
    return (lista != NULL) ? lista->total_itens : 0;
}

bool lista_gen_vazia(const ListaGen *lista) {
    return (lista == NULL || lista->total_itens == 0);
}

void lista_gen_percorrer(const ListaGen *lista, void (*callback)(ITEM *item, void *dados_extras), void *dados_extras) {
    if (lista == NULL || callback == NULL) return;

    for (Grupo *g = lista->raiz; g != NULL; g = g->prox) {
        for (Subgrupo *s = g->subgrupos; s != NULL; s = s->prox) {
            for (NoItem *ni = s->itens; ni != NULL; ni = ni->prox) {
                callback(ni->item, dados_extras);
            }
        }
    }
}
