#ifndef LISTA_SEQ_H
#define LISTA_SEQ_H

#include <stdbool.h>
#include "generic_item.h" // Certifique-se de que aponta para o seu header de itens

// Opaco ou estruturado conforme sua preferência, aqui deixamos a struct visível ou no .c
typedef struct lista_ ListaSeq;

// Funções principais
ListaSeq* lista_seq_criar(void);
void lista_seq_apagar(ListaSeq **lista);
bool lista_seq_inserir(ListaSeq *lista, ITEM *item);
ITEM* lista_seq_remover_fim(ListaSeq *lista);
int lista_seq_tamanho(const ListaSeq *lista);
bool lista_seq_vazia(const ListaSeq *lista);

// Função de percurso genérica (muito útil para a busca sem duplicar código!)
void lista_seq_percorrer(const ListaSeq *lista, void (*callback)(ITEM *item, void *dados_extras), void *dados_extras);

#endif