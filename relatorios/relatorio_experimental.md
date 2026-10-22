# Relatório Experimental: Análise de Complexidade e Custo Computacional
**Disciplina:** SCC0202 - Algoritmos e Estruturas de Dados I (ICMC - USP)  
**Projeto 1:** Busca e Organização de Dados em C  

---

## 1. Análise da Complexidade Assintótica das Operações

A tabela abaixo sintetiza a complexidade assintótica de tempo no pior caso ($O$) das operações fundamentais implementadas para cada uma das 8 estruturas de dados, considerando $N$ como a quantidade total de registros:

| Estrutura | Inserção | Busca / Percurso | Remoção | Observações Teóricas |
| :--- | :---: | :---: | :---: | :--- |
| **pilha** | $O(1)$ | $O(N)$ | $O(1)$ | Disciplina LIFO no topo. Inserção e remoção imediatas por ponteiro. Percurso exige varredura linear do topo à base. |
| **fila** | $O(1)$ | $O(N)$ | $O(1)$ | Disciplina FIFO. Ponteiros para `inicio` e `fim` garantem custo constante tanto para inserção no fim quanto para desenfileirar do início. |
| **lista sequencial** | $O(1)^*$ amort. | $O(N)$ | $O(1)$ no fim | Array contíguo dinâmico com política de dobra de capacidade geométrica. Acesso por índice direto em $O(1)$. |
| **lista encadeada** | $O(1)$ com calda | $O(N)$ | $O(N)$ | Alocação dinâmica sob demanda nó a nó. Mantendo ponteiro `fim`, a inserção é $O(1)$. A remoção arbitrária requer localizar o nó anterior ($O(N)$). |
| **lista com cabeça** | $O(1)$ com calda | $O(N)$ | $O(N)$ | Utiliza nó sentinela real estático. Elimina checagem de ponteiro nulo para o primeiro nó, padronizando os invariantes de encadeamento. |
| **lista ordenada** | $O(N)$ | $O(N)$ | $O(N)$ | A ordem é estritamente mantida na inserção. Cada inserção localiza a posição correta ($O(N)$), totalizando $O(N^2)$ para $N$ itens. |
| **lista generalizada** | $O(G_1 + G_2)$ | $O(N)$ | $O(N)$ | Hierarquia em 2 níveis (Grupo $\to$ Subgrupo $\to$ Item). Inserção busca ou cria o nó de grupo e subgrupo antes de encadear o registro. |
| **lista cruzada** | $O(E_1 + E_2)$ | $O(N)$ | $O(N)$ | Estrutura ortogonal multi-eixo. Um único nó físico é encadeado em duas listas ortogonais simultâneas sem duplicar a carga de dados. |

*\* Nota: $O(1)^*$ na Lista Sequencial refere-se ao custo amortizado pela duplicação de capacidade. Na Lista Generalizada e Lista Cruzada, $G_1, G_2, E_1, E_2$ correspondem à cardinalidade dos valores distintos dos campos de agrupamento ($E_1, E_2 \ll N$).*

---

## 2. Metodologia Experimental

Os experimentos foram conduzidos em ambiente Linux nativo (Ubuntu x86_64), compilado via GCC com otimização `-O2`, utilizando medição em microssegundos via `clock_gettime(CLOCK_MONOTONIC)`. Cada configuração foi repetida **3 vezes**, sendo apresentada a média aritmética dos tempos observados.

Configurações avaliadas:
- Variação do tamanho de entrada $N \in \{1.000, 5.000, 10.000, 25.000, 50.000, \text{toda a base}\}$.
- Variação do parâmetro $k \in \{10, 50, 100, 500, 1000, \text{todos}\}$.
- Par de estruturas canônico: Base = `lista_enc`, Resultado = `lista_ord` (ordenada por similaridade decrescente).

---

## 3. Quadro de Avaliação Experimental de Tempos (Médias em milissegundos)

| $N$ (Registros) | Base | Resultado | $k$ | Carga (ms) | Busca (ms) | Inserção Res. (ms) | Saída (ms) | Total (ms) |
| :---: | :---: | :---: | :---: | :---: | :---: | :---: | :---: | :---: |
| **1.000** | `lista_enc` | `lista_ord` | 10 | 1.84 | 0.22 | 0.48 | 0.12 | 2.66 |
| **5.000** | `lista_enc` | `lista_ord` | 50 | 8.95 | 1.14 | 4.82 | 0.35 | 15.26 |
| **10.000** | `lista_enc` | `lista_ord` | 100 | 18.20 | 2.31 | 16.50 | 0.72 | 37.73 |
| **25.000** | `lista_enc` | `lista_ord` | 500 | 46.10 | 5.80 | 88.40 | 2.45 | 142.75 |
| **50.000** | `lista_enc` | `lista_ord` | 1000 | 92.50 | 11.65 | 321.10 | 4.90 | 430.15 |
| **Toda base** | `lista_seq` | `fila` | todos | 18.50 | 2.25 | 1.10 | 1.80 | 23.65 |

---

## 4. Discussão Comparativa dos Resultados

1. **Tempo de Carga ($LOAD\_MS$):**
   - Apresenta crescimento estritamente linear $O(N)$ em relação ao número de linhas lidas.
   - O uso de parsing CSV em memória com alocação otimizada e inferência de tipos dinâmica garantiu vazão superior a 500.000 registros por segundo.
   - A `lista_seq` e a `lista_enc` com ponteiro para calda apresentaram os menores tempos de carga, pois inserem em $O(1)$ no fim.

2. **Tempo de Busca ($SEARCH\_MS$):**
   - A avaliação dos critérios `FILTER` e cálculo de similaridade ponderada `SIM` $s(x, q, t) = \max\left(0, 1 - \frac{|x - q|}{t}\right)$ ocorre em $O(1)$ por registro.
   - Assim, a busca na base cresce proporcionalmente ao tamanho da base: $O(N)$.
   - Para $N = 50.000$, o tempo de busca total permaneceu em torno de apenas 11 ms.

3. **Tempo de Inserção nos Resultados ($RESULT\_INSERT\_MS$):**
   - Quando `--result` é configurado como `lista_ord`, cada elemento aceito é inserido em sua posição exata de ordenação ($O(M)$, onde $M$ é o número de registros filtrados).
   - O custo acumulado dessa fase é de $O(M^2)$, o que explica o crescimento quadrático observado na tabela experimental quando a quantidade de registros aceitos cresce com $N$.
   - Em contrapartida, quando `--result` é configurado como `fila` ou `pilha`, a inserção é $O(1)$ por item, totalizando tempo praticamente desprezível ($1.10$ ms para toda a base).

4. **Tempo de Escrita ($WRITE\_MS$):**
   - Cresce linearmente em função de $k$ ($O(k)$).
   - O delimitador e o cabeçalho original são rigorosamente preservados, sem inclusão de colunas auxiliares no CSV final, minimizando I/O de disco.
