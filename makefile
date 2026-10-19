CC = gcc
CFLAGS = -Wall -Wextra -O2

# Caminhos dos tads
SRC_ITEM = libs/item/generic_item.c
SRC_SEARCH = libs/search/generic_search.c
SRC_PILHA = libs/pilha/pilha.c
SRC_FILA = libs/fila/fila.c
SRC_LISTA_SEQ = libs/lista_seq/lista_seq.c

# Caminho da main
SRC_MAIN = main.c

# Diretórios de include
INCLUDES = -Ilibs/item -Ilibs/search -Ilibs/pilha -Ilibs/fila -Ilibs/lista_seq

all:
	$(CC) $(INCLUDES) $(SRC_ITEM) $(SRC_SEARCH) $(SRC_PILHA) $(SRC_FILA) $(SRC_LISTA_SEQ) "$(SRC_MAIN)" $(CFLAGS) -o teste -lm

clean:
	rm -f teste