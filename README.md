Para executar, abra um terminal na raiz do projeto e execute um dos casos testes.

Casos testes:

./trabalho --reader nasa --input dados/asteroids.csv --base fila --result lista_gen --query consultas/q1.txt --output resultado.csv --top-k 10 --limit 10000

./trabalho --reader nasa --input dados/asteroids.csv --base lista_enc --result pilha --query consultas/q2.txt --output resultado.csv --top-k 20 --limit 10000

./trabalho --reader nasa --input dados/asteroids.csv --base lista_seq --result lista_ord --query consultas/q3.txt --output resultado.csv --top-k 100 --limit 10000

------------------------------------------------------------------

./trabalho --reader mortes --input dados/mortes.csv --base lista_enc --result lista_ord --query consultas/q_mortes1.txt --output resultado.csv --top-k 50 --limit 10000

./trabalho --reader mortes --input dados/mortes.csv --base lista_enc --result lista_ord --query consultas/q_mortes2.txt --output resultado.csv --top-k 10 --limit 10000
