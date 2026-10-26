# Busca em CSV com Estruturas de Dados (TADs)

Programa em C que lê um arquivo CSV, carrega os registros em uma **estrutura base**, aplica uma **consulta** (filtros e similaridade), guarda os registros que passaram em uma **estrutura de resultado** e exporta os top-n em um CSV. O objetivo é comparar o desempenho de diferentes TADs (pilha, fila e vários tipos de lista) na carga, na busca e na escrita.

## Estruturas suportadas

Podem ser usadas tanto em `--base` quanto em `--result`:

| Nome no parâmetro | Estrutura |
|---|---|
| `pilha` | Pilha dinâmica |
| `fila` | Fila dinâmica |
| `lista_seq` | Lista sequencial (vetor que cresce) |
| `lista_enc` | Lista simplesmente encadeada |
| `lista_cabeca` | Lista encadeada com nó cabeça |
| `lista_ord` | Lista ordenada (usa `ORDER` da consulta) |
| `lista_gen` | Lista generalizada (usa `GROUP` da consulta) |
| `lista_cruz` | Lista cruzada (usa `CROSS` da consulta) |

## Estrutura do projeto

```
.
├── main.c                  # Leitura dos parâmetros, carga, busca e exportação
├── estrutura.c / .h        # Interface única que despacha para cada TAD
├── pilha.c / .h
├── fila.c / .h
├── lista_seq.c / .h
├── lista_enc.c / .h
├── lista_cabeca.c / .h
├── lista_ord.c / .h
├── lista_gen.c / .h
├── lista_cruz.c / .h
├── generic_item.c / .h     # Item genérico, parser de CSV e inferência de tipos
├── generic_search.c / .h   # Leitura e avaliação das consultas
├── reader.c / .h           # Leitor de linhas do dataset
├── dados/                  # Datasets (.csv)
└── consultas/              # Arquivos de consulta (.txt)
```

## Execução

```bash
make all
```

```bash
./trabalho --reader <nome> --input <csv> --base <estrutura> --result <estrutura> \
           --query <consulta.txt> --output <saida.csv> [--top-k N] [--limit N]
```

| Parâmetro | Obrigatório | Descrição |
|---|---|---|
| `--reader` | sim | Nome do leitor do dataset (qualquer coisa, é um leitor genérico) |
| `--input` | sim | Caminho do CSV de entrada |
| `--base` | sim | Estrutura onde os dados do CSV são carregados |
| `--result` | sim | Estrutura que guarda os registros que atendem à consulta |
| `--query` | sim | Arquivo de consulta |
| `--output` | sim | CSV de saída |
| `--top-k` | não | Quantidade máxima de registros escritos na saída. `0` (padrão) escreve todos |
| `--limit` | não | Quantidade máxima de linhas lidas do CSV. `0` (padrão) lê o arquivo inteiro |

## Formato do arquivo de consulta

Uma instrução por linha. Linhas começadas por `#` são comentários. Os campos podem ser separados por espaço ou `|`, e aspas duplas protegem valores com espaços.

| Comando | Sintaxe | Descrição |
|---|---|---|
| `FILTER` | `FILTER coluna operador valor` | Mantém só registros que satisfazem a condição. Operadores: `==`, `!=`, `<`, `<=`, `>`, `>=` |
| `SIM` | `SIM coluna alvo tolerância peso` | Calcula similaridade numérica com o alvo (tolerância > 0, peso >= 0) |
| `ACCEPT` | `ACCEPT limiar` | Descarta registros com similaridade total abaixo do limiar |
| `ORDER` | `ORDER coluna [ASC\|DESC]` | Ordenação usada por `lista_ord`. A coluna `similarity` ordena pela similaridade calculada |
| `GROUP` | `GROUP coluna1 [coluna2]` | Chaves de agrupamento usadas por `lista_gen` |
| `CROSS` | `CROSS coluna1 [coluna2]` | Chaves dos dois eixos usadas por `lista_cruz` |

Exemplo:

```
# Asteroides perigosos e próximos de um diâmetro alvo
FILTER is_hazardous == true
SIM diameter_km 1.5 2.0 1.0
ACCEPT 0.6
ORDER similarity DESC
```

## Casos de teste

### Dataset de asteroides da NASA

```bash
./trabalho --reader nasa --input dados/asteroids.csv --base lista_enc --result lista_ord \
  --query consultas/q1.txt --output res_ex1.csv --top-k 5 --limit 1000

./trabalho --reader nasa --input dados/asteroids.csv --base pilha --result fila \
  --query consultas/q2.txt --output res_ex2.csv --top-k 10 --limit 1000

./trabalho --reader nasa --input dados/asteroids.csv --base lista_cabeca --result lista_cruz \
  --query consultas/q3.txt --output res_ex3.csv --top-k 0 --limit 0
```

### Dataset de homicídios na Bahia

```bash
./trabalho --reader mortes --input dados/mortes.csv --base lista_enc --result lista_ord \
  --query consultas/q_mortes1.txt --output res_ex4.csv --top-k 50 --limit 10000

./trabalho --reader mortes --input dados/mortes.csv --base lista_enc --result lista_ord \
  --query consultas/q_mortes2.txt --output res_ex5.csv --top-k 10 --limit 10000

./trabalho --reader mortes --input dados/mortes.csv --base lista_cabeca --result lista_ord \
  --query consultas/q_mortes3.txt --output res_ex6.csv --top-k 0 --limit 0
```

## Saída no terminal

Ao final da execução o programa imprime um resumo:

```
STATUS=OK
INPUT_RECORDS=...      # registros lidos e inseridos na estrutura base
MATCHED_RECORDS=...    # registros que passaram na consulta
OUTPUT_RECORDS=...     # registros escritos no CSV de saída
LOAD_MS=...            # tempo de carga do CSV na estrutura base
SEARCH_MS=...          # tempo gasto avaliando a consulta
RESULT_INSERT_MS=...   # tempo de inserção na estrutura de resultado
WRITE_MS=...           # tempo de escrita do CSV de saída
```

## Observações

- Os registros são compartilhados entre a estrutura base e a de resultado por contagem de referências, então a busca não copia os dados.
- Ao exportar, `pilha` e `fila` são consumidas (os itens são removidos conforme são escritos). As demais estruturas são apenas percorridas.
- Os tipos das colunas (inteiro, decimal, booleano, texto) são inferidos automaticamente a partir de cada valor do CSV.
- Arquivos com BOM UTF-8 no início (comuns no Windows) são tratados automaticamente, tanto no CSV quanto na consulta.
