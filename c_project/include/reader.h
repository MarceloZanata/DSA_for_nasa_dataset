#ifndef READER_H
#define READER_H

#include "generic_item.h"

typedef struct reader_t Reader;

typedef ITEM* (*ParseLineFn)(const char *linha, const CSVHeader *header);

struct reader_t {
    char *nome;
    ParseLineFn parse_line;
};

Reader* reader_obter(const char *nome);
void reader_liberar(Reader *r);

#endif
