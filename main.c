#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

// Inclua aqui os headers dos seus módulos (TADs, leitor, etc.)
// #include "generic_item.h"
// #include "pilha.h"
// #include "fila.h"
// ...

int main(int argc, char *argv[]) {
    // 1. Variáveis para armazenar os parâmetros de entrada
    char *reader = NULL;
    char *input = NULL;
    char *base_type = NULL;
    char *result_type = NULL;
    char *query = NULL;
    char *output = NULL;
    int top_k = 0;
    int limit = 0;

    // 2. Parsing dos argumentos da linha de comando (--flag valor)
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--reader") == 0 && i + 1 < argc) {
            reader = argv[++i];
        } else if (strcmp(argv[i], "--input") == 0 && i + 1 < argc) {
            input = argv[++i];
        } else if (strcmp(argv[i], "--base") == 0 && i + 1 < argc) {
            base_type = argv[++i];
        } else if (strcmp(argv[i], "--result") == 0 && i + 1 < argc) {
            result_type = argv[++i];
        } else if (strcmp(argv[i], "--query") == 0 && i + 1 < argc) {
            query = argv[++i];
        } else if (strcmp(argv[i], "--output") == 0 && i + 1 < argc) {
            output = argv[++i];
        } else if (strcmp(argv[i], "--top-k") == 0 && i + 1 < argc) {
            top_k = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--limit") == 0 && i + 1 < argc) {
            limit = atoi(argv[++i]);
        }
    }

    // Validação básica para garantir que os parâmetros obrigatórios foram passados
    if (!reader || !input || !base_type || !result_type || !query || !output) {
        fprintf(stderr, "Uso incorreto dos argumentos. Exemplo:\n");
        fprintf(stderr, "./trabalho --reader nasa --input dados/asteroids.csv --base lista_enc --result lista_ord --query consultas/q1.txt --output resultado.csv --top-k 100 --limit 10000\n");
        return 1;
    }

    // -------------------------------------------------------------
    // ESPAÇO PARA O FLUXO DO PROGRAMA (Implemente aos poucos)
    // -------------------------------------------------------------

    // Variáveis de métricas exigidas na saída padrão
    int input_records = 0;
    int matched_records = 0;
    int output_records = 0;
    double load_ms = 0.0;
    double search_ms = 0.0;
    double result_insert_ms = 0.0;
    double write_ms = 0.0;

    // PASSO 1: Inicializar a estrutura --base escolhida (pilha, fila, lista_enc, etc.)
    // void *base_structure = criar_estrutura(base_type);

    // PASSO 2: Carregar o dataset aplicando o --limit e medindo o tempo (LOAD_MS)
    // Exemplo: carregar_dados(input, limit, base_structure, &input_records, &load_ms);

    // PASSO 3: Inicializar a estrutura --result
    // void *result_structure = criar_estrutura(result_type);

    // PASSO 4: Ler o arquivo de consulta (--query) e executar a busca na --base,
    // medindo o tempo de busca (SEARCH_MS) e inserção no resultado (RESULT_INSERT_MS)
    // Exemplo: executar_busca(base_structure, query, result_structure, &matched_records, &search_ms, &result_insert_ms);

    // PASSO 5: Salvar os resultados no arquivo --output respeitando o --top-k,
    // medindo o tempo de escrita (WRITE_MS)
    // Exemplo: salvar_resultados(output, result_structure, top_k, &output_records, &write_ms);

    // PASSO 6: Liberar toda a memória alocada (estruturas, itens, etc.)
    // destruir_estrutura(base_structure, base_type);
    // destruir_estrutura(result_structure, result_type);

    // -------------------------------------------------------------
    // IMPRESSÃO DAS MÉTRICAS EXIGIDAS PELO PROFESSOR
    // -------------------------------------------------------------
    printf("STATUS=OK\n");
    printf("INPUT_RECORDS=%d\n", input_records);
    printf("MATCHED_RECORDS=%d\n", matched_records);
    printf("OUTPUT_RECORDS=%d\n", output_records);
    printf("LOAD_MS=%.3f\n", load_ms);
    printf("SEARCH_MS=%.3f\n", search_ms);
    printf("RESULT_INSERT_MS=%.3f\n", result_insert_ms);
    printf("WRITE_MS=%.3f\n", write_ms);

    return 0;
}