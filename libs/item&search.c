#include <stdio.h>
#include <stdlib.h>
#include "generic_item.h"
#include "generic_search.h"

int main(void) {
    int total_rows = 0;
    Item **dataset = read_csv_to_items("dados.csv", &total_rows);
    if (!dataset || total_rows == 0) {
        printf("Erro ao carregar o arquivo 'dados.csv' ou arquivo vazio.\n");
        return 1;
    }

    Query *q = parse_query_file("consulta.txt");
    if (!q) {
        printf("Erro ao carregar o arquivo 'consulta.txt'.\n");
        for (int i = 0; i < total_rows; i++) item_free(dataset[i]);
        free(dataset);
        return 1;
    }

    int out_count = 0;
    Item **results = execute_query(dataset, total_rows, q, &out_count);

    printf("\n=== RESULTADOS ENCONTRADOS: %d ===\n\n", out_count);
    for (int i = 0; i < out_count; i++) {
        printf("Registro [%d]:\n", i + 1);
        item_print(results[i]);
        printf("\n");
    }

    // Limpeza de memória
    free(results);
    query_free(q);
    for (int i = 0; i < total_rows; i++) {
        item_free(dataset[i]);
    }
    free(dataset);

    return 0;
}