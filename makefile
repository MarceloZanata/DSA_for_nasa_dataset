CC = gcc
CFLAGS = -Wall -Wextra -pedantic -O2

# Caminhos dos tads
SRC_ITEM = libs/item/generic_item.c
SRC_SEARCH = libs/search/generic_search.c
SRC_PILHA = libs/pilha/pilha.c
SRC_FILA = libs/fila/fila.c
SRC_LISTA_SEQ = libs/lista_seq/lista_seq.c
SRC_LISTA_ORD = libs/lista_ord/lista_ord.c
SRC_LISTA_CABECA = libs/lista_cabeca/lista_cabeca.c
SRC_LISTA_ENC = libs/lista_enc/lista_enc.c
SRC_LISTA_GEN = libs/lista_gen/lista_gen.c
SRC_LISTA_CRUZ = libs/lista_cruz/lista_cruz.c

# Caminho da main
SRC_MAIN = main.c

# Diretórios de include
INCLUDES = -Ilibs/item -Ilibs/search -Ilibs/pilha -Ilibs/fila -Ilibs/lista_seq -Ilibs/lista_ord/ -Ilibs/lista_cabeca/ -Ilibs/lista_enc/ -Ilibs/lista_gen/ -Ilibs/lista_cruz/

all:
	$(CC) $(INCLUDES) $(SRC_ITEM) $(SRC_SEARCH) $(SRC_PILHA) $(SRC_FILA) $(SRC_LISTA_SEQ) "$(SRC_MAIN)" $(CFLAGS) -o teste -lm

clean:
	rm -f teste