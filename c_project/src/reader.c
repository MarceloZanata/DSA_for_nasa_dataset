#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "reader.h"

// Retorna o item criado a partir da linha
static ITEM* reader_parse(const char *linha, const CSVHeader *header) {
    return item_criar_de_linha(linha, header);
}

// Ignora o tipo de leitor pois ele é genérico
Reader* reader_obter(const char *nome) {
    Reader *r = (Reader *) malloc(sizeof(Reader));
    if (!r) return NULL;

    r->nome = strdup(nome ? nome : "dataset");
    r->parse_line = reader_parse;
    return r;
}

// Destrutor do leitor
void reader_liberar(Reader *r) {
    if (!r) return;
    free(r->nome);
    free(r);
}
