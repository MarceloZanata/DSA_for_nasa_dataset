CC = gcc
CFLAGS = -Wall -Wextra -O2

# Caminhos
SRC_ITEM = libs/item/generic_item.c
SRC_SEARCH = libs/search/generic_search.c
SRC_MAIN = libs/item&search.c

# Diretórios de include
INCLUDES = -Ilibs/item -Ilibs/search

all:
	$(CC) $(INCLUDES) $(SRC_ITEM) $(SRC_SEARCH) "$(SRC_MAIN)" $(CFLAGS) -o teste -lm

clean:
	rm -f teste