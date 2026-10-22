#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "reader.h"

static ITEM* reader_parse(const char *linha, const CSVHeader *header) {
    return item_criar_de_linha(linha, header);
}

Reader* reader_obter(const char *nome) {
    Reader *r = (Reader *) malloc(sizeof(Reader));
    if (!r) return NULL;

    r->nome = strdup(nome ? nome : "dataset");
    r->parse_line = reader_parse;
    return r;
}

void reader_liberar(Reader *r) {
    if (!r) return;
    free(r->nome);
    free(r);
}
