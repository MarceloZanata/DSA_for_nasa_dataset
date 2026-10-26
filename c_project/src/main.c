#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include "generic_item.h"
#include "generic_search.h"
#include "estrutura.h"
#include "reader.h"

// Calcula a diferença do tempo inicial e final
static double diff_ms(struct timespec start, struct timespec end) {
    return (double)(end.tv_sec - start.tv_sec) * 1000.0 +
           (double)(end.tv_nsec - start.tv_nsec) / 1000000.0;
}

// Estrutura dos dados exibidos no final da execução
typedef struct {
    const Query *q;
    Estrutura *result;
    int matched_records;
    double search_ms;
    double result_insert_ms;
} SearchContext;


static void search_callback(ITEM *item, void *dados) {
    SearchContext *ctx = (SearchContext *) dados;
    struct timespec t0, t1, t2;
    double sim = 0.0;

    clock_gettime(CLOCK_MONOTONIC, &t0);
    bool matched = query_avaliar_item(item, ctx->q, &sim);
    clock_gettime(CLOCK_MONOTONIC, &t1);
    ctx->search_ms += diff_ms(t0, t1);

    if (matched) {
        ctx->matched_records++;
        item_set_similarity(item, sim);

        clock_gettime(CLOCK_MONOTONIC, &t1);
        item_retain(item);
        estrutura_inserir(ctx->result, item);
        clock_gettime(CLOCK_MONOTONIC, &t2);
        ctx->result_insert_ms += diff_ms(t1, t2);
    }
}

int main(int argc, char *argv[]) {
    char *reader = NULL;
    char *input = NULL;
    char *base_type = NULL;
    char *result_type = NULL;
    char *query = NULL;
    char *output = NULL;
    int top_k = 0;
    int limit = 0;

    // Leitor dos parâmetros
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

    // Valida os argumentos passados pelo usuário
    if (!reader || !input || !base_type || !result_type || !query || !output) {
        fprintf(stderr, "Uso incorreto dos argumentos. Exemplo:\n");
        fprintf(stderr, "./trabalho --reader nasa --input dados/asteroids.csv --base lista_enc --result lista_ord --query consultas/q1.txt --output resultado.csv --top-k 100 --limit 10000\n");
        return 1;
    }

    // Lê o arquivo de busca
    Query *q = parse_query_file(query);
    if (!q) {
        fprintf(stderr, "Erro ao abrir ou processar arquivo de consulta: %s\n", query);
        return 1;
    }

    // Inicializa o leitor genérico
    Reader *r = reader_obter(reader);
    if (!r) {
        fprintf(stderr, "Erro ao instanciar leitor: %s\n", reader);
        query_free(q);
        return 1;
    }

    // Abre o CSV original para leitura
    FILE *f_in = fopen(input, "r");
    if (!f_in) {
        fprintf(stderr, "Erro ao abrir arquivo de entrada: %s\n", input);
        reader_liberar(r);
        query_free(q);
        return 1;
    }

    // Leitor do cabeçalho do arquivo CSV
    CSVHeader *header = csv_header_ler(f_in);
    if (!header) {
        fprintf(stderr, "Erro ao ler cabecalho de: %s\n", input);
        fclose(f_in);
        reader_liberar(r);
        query_free(q);
        return 1;
    }

    // Cria a estrutura de dados de acordo com a estrutura passada no parametro --base
    Estrutura *base = estrutura_criar(base_type, q);
    if (!base) {
        fprintf(stderr, "Estrutura base desconhecida ou invalida: %s\n", base_type);
        csv_header_free(header);
        fclose(f_in);
        reader_liberar(r);
        query_free(q);
        return 1;
    }

    int input_records = 0;
    struct timespec t_start, t_end;

    // Inicializa a contagem do tempo
    clock_gettime(CLOCK_MONOTONIC, &t_start);
    char *linha;

    // Insere na estrutura de dados item por item lidos em cada linha do CSV
    while ((limit == 0 || input_records < limit) && (linha = csv_ler_linha(f_in)) != NULL) {
        ITEM *item = r->parse_line(linha, header);
        free(linha);
        if (!item) continue;

        if (estrutura_inserir(base, item)) {
            input_records++;
        } else {
            item_free(item);
        }
    }

    // Calcula o tempo necessário para carregar os arquivos
    clock_gettime(CLOCK_MONOTONIC, &t_end);
    double load_ms = diff_ms(t_start, t_end);
    fclose(f_in);

    // Cria a estrutura escolhida no parâmetro de output
    Estrutura *result = estrutura_criar(result_type, q);
    if (!result) {
        fprintf(stderr, "Estrutura result desconhecida ou invalida: %s\n", result_type);
        estrutura_apagar(&base);
        csv_header_free(header);
        reader_liberar(r);
        query_free(q);
        return 1;
    }


    SearchContext s_ctx;
    s_ctx.q = q;
    s_ctx.result = result;
    s_ctx.matched_records = 0;
    s_ctx.search_ms = 0.0;
    s_ctx.result_insert_ms = 0.0;

    // Percorre a estrutura com o que será buscado e a função que verifica se deu match
    estrutura_percorrer(base, search_callback, &s_ctx);

    // Abre o arquivo de saida para escrita
    FILE *f_out = fopen(output, "w");
    if (!f_out) {
        fprintf(stderr, "Erro ao abrir arquivo de saida: %s\n", output);
        estrutura_apagar(&result);
        estrutura_apagar(&base);
        csv_header_free(header);
        reader_liberar(r);
        query_free(q);
        return 1;
    }
    
    // Calcula o tempo de escrita e escreve no arquivo
    clock_gettime(CLOCK_MONOTONIC, &t_start);
    csv_header_escrever(f_out, header);
    int output_records = estrutura_exportar_top_k(result, f_out, top_k);
    fclose(f_out);
    clock_gettime(CLOCK_MONOTONIC, &t_end);
    double write_ms = diff_ms(t_start, t_end);

    // Libera toda a memória que estava sendo utiliazada
    estrutura_apagar(&result);
    estrutura_apagar(&base);
    csv_header_free(header);
    reader_liberar(r);
    query_free(q);

    // Printa as saidas
    printf("STATUS=OK\n");
    printf("INPUT_RECORDS=%d\n", input_records);
    printf("MATCHED_RECORDS=%d\n", s_ctx.matched_records);
    printf("OUTPUT_RECORDS=%d\n", output_records);
    printf("LOAD_MS=%.3f\n", load_ms);
    printf("SEARCH_MS=%.3f\n", s_ctx.search_ms);
    printf("RESULT_INSERT_MS=%.3f\n", s_ctx.result_insert_ms);
    printf("WRITE_MS=%.3f\n", write_ms);

    return 0;
}
