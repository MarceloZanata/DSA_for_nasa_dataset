# Relatório do Projeto: Busca e Organização de Dados
**Disciplina:** SCC0202 - Algoritmos e Estruturas de Dados I (ICMC - USP)  
**Projeto 1:** Busca e Organização de Dados em C  

---

## 1. Organização do Código e Módulos

O projeto foi arquitetado segundo as melhores práticas de encapsulamento e modularização de Tipos Abstratos de Dados (TADs) em C99 estrito (`-Wall -Wextra -pedantic`), mantendo isolamento absoluto entre lógica dos dados, leitores e estruturas de armazenamento:

```
├── Makefile                    # Compilação unificada gerando o executável ./trabalho
├── c_project/
│   ├── Makefile                # Build das regras de compilação C
│   ├── include/                # Headers públicos (.h) dos TADs e serviços
│   │   ├── generic_item.h      # TAD ITEM genérico com suporte a tipos e ref counting
│   │   ├── generic_search.h    # Motor de consulta (FILTER, SIM, ACCEPT, ORDER, GROUP, CROSS)
│   │   ├── estrutura.h         # Despachante polimórfico unificado (--base e --result)
│   │   ├── reader.h            # Interface modular de leitores de datasets (--reader)
│   │   ├── pilha.h             # TAD Pilha dinâmica encadeada (LIFO)
│   │   ├── fila.h              # TAD Fila dinâmica com ponteiros inicio/fim (FIFO)
│   │   ├── lista_seq.h         # TAD Lista Sequencial com realocação dinâmica
│   │   ├── lista_enc.h         # TAD Lista Simplesmente Encadeada
│   │   ├── lista_cabeca.h      # TAD Lista com Nó Cabeça/Sentinela real
│   │   ├── lista_ord.h         # TAD Lista Ordenada com inserção ordenada estrita
│   │   ├── lista_gen.h         # TAD Lista Generalizada (Árvore hierárquica Grupo -> Subgrupo)
│   │   └── lista_cruz.h        # TAD Lista Cruzada (Matriz ortogonal de nós compartilhados)
│   └── src/                    # Implementações (.c)
├── dados/
│   ├── asteroids.csv           # Base da Parte 1 (Aproximação de Asteroides - NASA)
│   └── compras.csv             # Base da Parte 2 (Dados Abertos - Compras Públicas Federais)
└── consultas/
    ├── q1.txt, q2.txt, q3.txt  # Consultas para a Parte 1
    └── q_compras1.txt, q_compras2.txt # Consultas para a Parte 2
```

---

## 2. Implementação das 8 Estruturas de Dados (TADs)

1. **Pilha (`pilha.c`)**: Estrutura dinâmica encadeada com disciplina LIFO (*Last In, First Out*). Possui ponteiro para o `topo` e contagem de elementos. No desempilhamento para exportação, o elemento mais recente é processado primeiro.
2. **Fila (`fila.c`)**: Estrutura dinâmica com disciplina FIFO (*First In, First Out*), mantendo ponteiros de acesso $O(1)$ para `inicio` e `fim`.
3. **Lista Sequencial (`lista_seq.c`)**: Armazenamento contíguo em vetor dinâmico de ponteiros para itens com política de duplicação geométrica de capacidade (`realloc`), garantindo custo amortizado $O(1)$ na inserção.
4. **Lista Encadeada (`lista_enc.c`)**: Lista encadeada linear com nós contendo ponteiro para o item e ponteiro para o próximo nó.
5. **Lista com Cabeça (`lista_cabeca.c`)**: Atende rigorosamente ao requisito de possuir um **nó cabeça/sentinela real alocado** em memória (`cabeca->item == NULL`). Todas as operações de inserção, remoção e percurso utilizam essa sentinela, eliminando casos especiais para listas vazias.
6. **Lista Ordenada (`lista_ord.c`)**: Atende estritamente à regra de que **a ordenação é realizada durante a inserção** (custo $O(N)$ por item, total $O(N^2)$ para $N$ itens), nunca inserindo desordenado para ordenar ao final. Suporta campos numéricos, alfanuméricos e o pseudocampo especial `similarity`.
7. **Lista Generalizada (`lista_gen.c`)**: Estrutura multinível hierárquica que agrupa registros dinamicamente em dois níveis conforme o comando `GROUP` (ex: `threat_category` -> `year`). Cada grupo possui uma sublista de subgrupos, que por sua vez contém a lista encadeada de itens.
8. **Lista Cruzada (`lista_cruz.c`)**: Matriz esparsa ortogonal em que **o mesmo registro participa de duas dimensões simultaneamente sem duplicação de memória**. Cada nó (`NoCruzado`) contém dois ponteiros independentes de encadeamento: `prox_eixo1` e `prox_eixo2`, ligados aos respectivos cabeçalhos de eixo. A liberação de memória é realizada apenas por um dos eixos com auxílio de contagem de referências no `ITEM`.

---

## 3. Gestão de Memória e Compartilhamento de Registros

Para evitar vazamentos de memória e ao mesmo tempo permitir que a estrutura `--base` e a estrutura `--result` referenciem os mesmos registros (ou subconjuntos deles):
- O tipo abstrato `ITEM` implementa **contagem de referências (`item->refs`)**.
- Ao ser carregado do CSV para a base, o item é instanciado com `refs = 1`.
- Ao ser aprovado no filtro da busca e inserido em `--result`, a função `item_retain(item)` incrementa o contador para `2`.
- Quando `--base` ou `--result` são destruídas, a função `item_free(item)` decrementa o contador, liberando buffers de texto e estrutura apenas quando `refs == 0`.
- Isso foi validado com **AddressSanitizer** (`-fsanitize=address,undefined`), garantindo **zero vazamentos de memória e zero acessos inválidos**.

---

## 4. Seis Exemplos Completos de Execução

### Exemplo 1 (Parte 1 - NASA: Lista Encadeada -> Lista Ordenada por Similaridade)
- **Comando:**
  ```bash
  ./trabalho --reader nasa --input dados/asteroids.csv --base lista_enc --result lista_ord --query consultas/q1.txt --output res_ex1.csv --top-k 5 --limit 1000
  ```
- **Consulta (`consultas/q1.txt`):**
  ```text
  FILTER|is_future_event|==|true
  FILTER|distance_au|<=|0.08
  SIM|velocity_km_s|15.0|10.0|1.0
  SIM|risk_score|0.70|0.30|2.0
  ACCEPT|0.60
  ORDER|similarity|DESC
  GROUP|threat_category|year
  CROSS|threat_category|year
  ```
- **Saída Padrão (stdout):**
  ```text
  STATUS=OK
  INPUT_RECORDS=20
  MATCHED_RECORDS=12
  OUTPUT_RECORDS=5
  LOAD_MS=0.109
  SEARCH_MS=0.005
  RESULT_INSERT_MS=0.003
  WRITE_MS=0.075
  ```
- **Resultado Obtido:** Os primeiros 5 registros em `res_ex1.csv` correspondem aos asteroides futuros com maior proximidade dos valores ideais de velocidade (15 km/s) e risco (0.70), ordenados em ordem decrescente de similaridade (liderados por `2035 NX` e `2029 WP`).

---

### Exemplo 2 (Parte 1 - NASA: Pilha -> Fila com Lista Sentry)
- **Comando:**
  ```bash
  ./trabalho --reader nasa --input dados/asteroids.csv --base pilha --result fila --query consultas/q2.txt --output res_ex2.csv --top-k 10 --limit 1000
  ```
- **Consulta (`consultas/q2.txt`):**
  ```text
  FILTER|on_sentry_list|==|true
  SIM|sentry_diameter_km|0.50|0.50|1.0
  ACCEPT|0.30
  ORDER|sentry_diameter_km|DESC
  GROUP|threat_category|year
  CROSS|threat_category|year
  ```
- **Saída Padrão (stdout):**
  ```text
  STATUS=OK
  INPUT_RECORDS=20
  MATCHED_RECORDS=6
  OUTPUT_RECORDS=6
  LOAD_MS=0.105
  SEARCH_MS=0.003
  RESULT_INSERT_MS=0.002
  WRITE_MS=0.054
  ```
- **Resultado Obtido:** Filtra asteroides oficialmente monitorados pelo sistema Sentry da NASA. A base em pilha descarrega e a fila preserva a ordem sequencial de enfileiramento dos itens encontrados.

---

### Exemplo 3 (Parte 1 - NASA: Lista com Cabeça -> Lista Cruzada para Eventos Passados)
- **Comando:**
  ```bash
  ./trabalho --reader nasa --input dados/asteroids.csv --base lista_cabeca --result lista_cruz --query consultas/q3.txt --output res_ex3.csv --top-k 0 --limit 0
  ```
- **Consulta (`consultas/q3.txt`):**
  ```text
  FILTER|is_past_event|==|true
  FILTER|distance_au|<=|0.05
  ORDER|close_approach_date|DESC
  GROUP|panic_level|year
  CROSS|threat_category|year
  ```
- **Saída Padrão (stdout):**
  ```text
  STATUS=OK
  INPUT_RECORDS=20
  MATCHED_RECORDS=4
  OUTPUT_RECORDS=4
  LOAD_MS=0.101
  SEARCH_MS=0.002
  RESULT_INSERT_MS=0.004
  WRITE_MS=0.048
  ```
- **Resultado Obtido:** Demonstra o percurso ortogonal da Lista Cruzada indexada simultaneamente por `threat_category` e `year`. A saída contém todos os 4 asteroides históricos que passaram a menos de 0.05 UA da Terra (`2008 EV5`, `2020 CD3`, `2024 MK`, `2024 PT5`).

---

### Exemplo 4 (Parte 2 - Dados Abertos: Lista Sequencial -> Lista Generalizada)
- **Comando:**
  ```bash
  ./trabalho --reader compras --input dados/compras.csv --base lista_seq --result lista_gen --query consultas/q_compras1.txt --output res_ex4.csv --top-k 10 --limit 0
  ```
- **Consulta (`consultas/q_compras1.txt`):**
  ```text
  FILTER|situacao_concluida|==|true
  FILTER|valor_homologado|>=|500000.0
  SIM|indice_competitividade|0.80|0.20|2.0
  SIM|desconto_obtido|15.0|10.0|1.0
  ACCEPT|0.50
  ORDER|valor_homologado|DESC
  GROUP|categoria_despesa|ano
  CROSS|categoria_despesa|orgao_uf
  ```
- **Saída Padrão (stdout):**
  ```text
  STATUS=OK
  INPUT_RECORDS=15
  MATCHED_RECORDS=9
  OUTPUT_RECORDS=9
  LOAD_MS=0.115
  SEARCH_MS=0.004
  RESULT_INSERT_MS=0.005
  WRITE_MS=0.082
  ```
- **Resultado Obtido:** As compras públicas federais concluídas de alto valor são agrupadas hierarquicamente por categoria (`Saude`, `Educacao`, `Infraestrutura`, `Tecnologia`) e subagrupadas por ano fiscal (`2024`, `2023`).

---

### Exemplo 5 (Parte 2 - Dados Abertos: Fila -> Lista Cruzada para Auditoria)
- **Comando:**
  ```bash
  ./trabalho --reader compras --input dados/compras.csv --base fila --result lista_cruz --query consultas/q_compras2.txt --output res_ex5.csv --top-k 5 --limit 0
  ```
- **Consulta (`consultas/q_compras2.txt`):**
  ```text
  FILTER|tem_recurso|==|true
  SIM|nivel_risco_auditoria|0.80|0.20|2.0
  ACCEPT|0.60
  ORDER|nivel_risco_auditoria|DESC
  GROUP|categoria_despesa|ano
  CROSS|categoria_despesa|orgao_uf
  ```
- **Saída Padrão (stdout):**
  ```text
  STATUS=OK
  INPUT_RECORDS=15
  MATCHED_RECORDS=3
  OUTPUT_RECORDS=3
  LOAD_MS=0.098
  SEARCH_MS=0.003
  RESULT_INSERT_MS=0.003
  WRITE_MS=0.041
  ```
- **Resultado Obtido:** Filtra licitações com recursos judiciais/administrativos pendentes e alto risco de auditoria, cruzando as categorias com as UFs de origem (`DF`, `RJ`, `RS`).

---

### Exemplo 6 (Parte 2 - Dados Abertos: Lista com Cabeça -> Lista Ordenada por Risco)
- **Comando:**
  ```bash
  ./trabalho --reader compras --input dados/compras.csv --base lista_cabeca --result lista_ord --query consultas/q_compras2.txt --output res_ex6.csv --top-k 0 --limit 0
  ```
- **Saída Padrão (stdout):**
  ```text
  STATUS=OK
  INPUT_RECORDS=15
  MATCHED_RECORDS=3
  OUTPUT_RECORDS=3
  LOAD_MS=0.103
  SEARCH_MS=0.003
  RESULT_INSERT_MS=0.002
  WRITE_MS=0.044
  ```
- **Resultado Obtido:** Lista ordenada gerada com inserção estritamente ordenada por `nivel_risco_auditoria` descendente, destacando os contratos mais críticos em primeiro lugar no arquivo CSV.
