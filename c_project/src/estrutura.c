
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "estrutura.h"
#include "pilha.h"
#include "fila.h"
#include "lista_seq.h"
#include "lista_enc.h"
#include "lista_cabeca.h"
#include "lista_ord.h"
#include "lista_gen.h"
#include "lista_cruz.h"

// Estrutura escolhida na base ou no output
struct estrutura_t {
    TipoEstrutura tipo;
    char *nome;
    void *instancia;
};

//Identifica o tipo da estrutura de dados requisitada com base no enum localizado no .h
static TipoEstrutura identificar_tipo(const char *nome) {
    if (!nome) return EST_DESCONHECIDA;
    if (strcmp(nome, "pilha") == 0) return EST_PILHA;
    if (strcmp(nome, "fila") == 0) return EST_FILA;
    if (strcmp(nome, "lista_seq") == 0) return EST_LISTA_SEQ;
    if (strcmp(nome, "lista_enc") == 0) return EST_LISTA_ENC;
    if (strcmp(nome, "lista_cabeca") == 0) return EST_LISTA_CABECA;
    if (strcmp(nome, "lista_ord") == 0) return EST_LISTA_ORD;
    if (strcmp(nome, "lista_gen") == 0) return EST_LISTA_GEN;
    if (strcmp(nome, "lista_cruz") == 0) return EST_LISTA_CRUZ;
    return EST_DESCONHECIDA;
}

// Construtor
Estrutura* estrutura_criar(const char *nome, const Query *q) {
    TipoEstrutura tipo = identificar_tipo(nome);
    if (tipo == EST_DESCONHECIDA) return NULL; // Valida o tipo da estrutura

    Estrutura *est = (Estrutura *) malloc(sizeof(Estrutura));
    if (!est) return NULL;

    est->tipo = tipo;
    est->nome = strdup(nome);
    est->instancia = NULL;

    // A partir do tipo, inicializa a respectiva estrutura
    switch (tipo) {
        case EST_PILHA:
            est->instancia = pilha_criar();
            break;
        case EST_FILA:
            est->instancia = fila_criar();
            break;
        case EST_LISTA_SEQ:
            est->instancia = lista_seq_criar();
            break;
        case EST_LISTA_ENC:
            est->instancia = lista_enc_criar();
            break;
        case EST_LISTA_CABECA:
            est->instancia = lista_cabeca_criar();
            break;
        case EST_LISTA_ORD: {
            bool desc = false;
            const char *order_key = query_get_order_key(q, &desc);
            est->instancia = lista_ord_criar(order_key, desc);
            break;
        }
        case EST_LISTA_GEN: {
            char k1[128] = "x", k2[128] = "y";
            query_get_group_keys(q, k1, k2, sizeof(k1));
            est->instancia = lista_gen_criar(k1, k2);
            break;
        }
        case EST_LISTA_CRUZ: {
            char k1[128] = "x", k2[128] = "y";
            query_get_cross_keys(q, k1, k2, sizeof(k1));
            est->instancia = lista_cruz_criar(k1, k2);
            break;
        }
        default:
            break;
    }

    if (!est->instancia) {
        free(est->nome);
        free(est);
        return NULL;
    }

    // Retorna a estrutura ao usuário
    return est;
}

// Insere item na estrutura
bool estrutura_inserir(Estrutura *est, ITEM *item) {
    if (!est || !est->instancia || !item) return false; // o ponteiro de estrutura e item são validados

    // utiliza o insere de acordo com a estrutra usada no momento
    switch (est->tipo) {
        case EST_PILHA:
            return pilha_empilhar((PILHA *) est->instancia, item);
        case EST_FILA:
            return fila_inserir((FILA *) est->instancia, item);
        case EST_LISTA_SEQ:
            return lista_seq_inserir((ListaSeq *) est->instancia, item);
        case EST_LISTA_ENC:
            return lista_enc_inserir((ListaEnc *) est->instancia, item);
        case EST_LISTA_CABECA:
            return lista_cabeca_inserir((ListaCabeca *) est->instancia, item);
        case EST_LISTA_ORD:
            return lista_ord_inserir((ListaOrd *) est->instancia, item);
        case EST_LISTA_GEN:
            return lista_gen_inserir((ListaGen *) est->instancia, item);
        case EST_LISTA_CRUZ:
            return lista_cruz_inserir((ListaCruz *) est->instancia, item);
        default:
            return false;
    }
}

// Retorna o tamanho
int estrutura_tamanho(const Estrutura *est) {
    if (!est || !est->instancia) return 0;

    // Pega o tamanho de acordo com a estrutura utilizada
    switch (est->tipo) {
        case EST_PILHA:
            return pilha_tamanho((PILHA *) est->instancia);
        case EST_FILA:
            return fila_tamanho((FILA *) est->instancia);
        case EST_LISTA_SEQ:
            return lista_seq_tamanho((ListaSeq *) est->instancia);
        case EST_LISTA_ENC:
            return lista_enc_tamanho((ListaEnc *) est->instancia);
        case EST_LISTA_CABECA:
            return lista_cabeca_tamanho((ListaCabeca *) est->instancia);
        case EST_LISTA_ORD:
            return lista_ord_tamanho((ListaOrd *) est->instancia);
        case EST_LISTA_GEN:
            return lista_gen_tamanho((ListaGen *) est->instancia);
        case EST_LISTA_CRUZ:
            return lista_cruz_tamanho((ListaCruz *) est->instancia);
        default:
            return 0;
    }
}

// Percorre a estrutura
void estrutura_percorrer(const Estrutura *est, void (*callback)(ITEM *item, void *dados_extras), void *dados_extras) {
    if (!est || !est->instancia || !callback) return;

    // Percorre a estrutura de acordo com o seu tipo, utiliza o callback para
    //  percorrer de forma genérica: serve para printar ou salvar dados em arquivo
    switch (est->tipo) {
        case EST_PILHA:
            pilha_percorrer((PILHA *) est->instancia, callback, dados_extras);
            break;
        case EST_FILA:
            fila_percorrer((FILA *) est->instancia, callback, dados_extras);
            break;
        case EST_LISTA_SEQ:
            lista_seq_percorrer((ListaSeq *) est->instancia, callback, dados_extras);
            break;
        case EST_LISTA_ENC:
            lista_enc_percorrer((ListaEnc *) est->instancia, callback, dados_extras);
            break;
        case EST_LISTA_CABECA:
            lista_cabeca_percorrer((ListaCabeca *) est->instancia, callback, dados_extras);
            break;
        case EST_LISTA_ORD:
            lista_ord_percorrer((ListaOrd *) est->instancia, callback, dados_extras);
            break;
        case EST_LISTA_GEN:
            lista_gen_percorrer((ListaGen *) est->instancia, callback, dados_extras);
            break;
        case EST_LISTA_CRUZ:
            lista_cruz_percorrer_eixo1((ListaCruz *) est->instancia, callback, dados_extras);
            break;
        default:
            break;
    }
}

// Estrutura de exportar para o arquivo
typedef struct {
    FILE *out;
    int max_k;
    int count;
} ExportContext;

// Função callback é acionada a cada item percorrido da estrutura de dados
static void exportar_callback(ITEM *item, void *dados) {
    ExportContext *ctx = (ExportContext *) dados;   // Envia de volta para a estrutura de dados
    if (ctx->max_k > 0 && ctx->count >= ctx->max_k) return; // Se chegou ao k, encerra
    item_escrever_csv(ctx->out, item);  // Escreve o item n de k no CSV
    ctx->count++;
}

// Função principal de escrever o arquivo de saida
int estrutura_exportar_top_k(Estrutura *est, FILE *arquivo_saida, int top_k) {
    if (!est || !est->instancia || !arquivo_saida) return 0;

    ExportContext ctx;
    ctx.out = arquivo_saida;
    ctx.max_k = top_k;
    ctx.count = 0;

    // Para pilha e fila, podemos remover conforme a regra de acesso do TAD
    if (est->tipo == EST_PILHA) {
        PILHA *p = (PILHA *) est->instancia;
        while (!pilha_vazia(p) && (top_k == 0 || ctx.count < top_k)) {
            ITEM *item = pilha_desempilhar(p);
            item_escrever_csv(arquivo_saida, item);
            item_free(item);
            ctx.count++;
        }
        return ctx.count;
    } else if (est->tipo == EST_FILA) {
        FILA *f = (FILA *) est->instancia;
        while (!fila_vazia(f) && (top_k == 0 || ctx.count < top_k)) {
            ITEM *item = fila_remover(f);
            item_escrever_csv(arquivo_saida, item);
            item_free(item);
            ctx.count++;
        }
        return ctx.count;
    }

    /* Para listas (seq, enc, cabeca, ord, gen, cruz), percurso natural */
    estrutura_percorrer(est, exportar_callback, &ctx);
    return ctx.count;
}

// Destrutor
void estrutura_apagar(Estrutura **est) {
    if (!est || !*est) return;  // Valida a estrutura

    // Apenas chama o destrutor específico do tipo de estrutura
    Estrutura *e = *est;
    if (e->instancia) {
        switch (e->tipo) {
            case EST_PILHA: {
                PILHA *p = (PILHA *) e->instancia;
                pilha_apagar(&p);
                break;
            }
            case EST_FILA: {
                FILA *f = (FILA *) e->instancia;
                fila_apagar(&f);
                break;
            }
            case EST_LISTA_SEQ: {
                ListaSeq *l = (ListaSeq *) e->instancia;
                lista_seq_apagar(&l);
                break;
            }
            case EST_LISTA_ENC: {
                ListaEnc *l = (ListaEnc *) e->instancia;
                lista_enc_apagar(&l);
                break;
            }
            case EST_LISTA_CABECA: {
                ListaCabeca *l = (ListaCabeca *) e->instancia;
                lista_cabeca_apagar(&l);
                break;
            }
            case EST_LISTA_ORD: {
                ListaOrd *l = (ListaOrd *) e->instancia;
                lista_ord_apagar(&l);
                break;
            }
            case EST_LISTA_GEN: {
                ListaGen *l = (ListaGen *) e->instancia;
                lista_gen_apagar(&l);
                break;
            }
            case EST_LISTA_CRUZ: {
                ListaCruz *l = (ListaCruz *) e->instancia;
                lista_cruz_apagar(&l);
                break;
            }
            default:
                break;
        }
    }

    free(e->nome);
    free(e);
    *est = NULL;
}

// Retorna o nome da estrutura
const char* estrutura_nome(const Estrutura *est) {
    return est ? est->nome : "desconhecida";
}
