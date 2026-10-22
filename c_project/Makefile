CC = gcc
CFLAGS = -Wall -Wextra -pedantic -std=c99 -O2 -Iinclude -D_DEFAULT_SOURCE -D_POSIX_C_SOURCE=200809L
LDFLAGS = -lm

SRC = src/generic_item.c \
      src/generic_search.c \
      src/fila.c \
      src/pilha.c \
      src/lista_seq.c \
      src/lista_enc.c \
      src/lista_cabeca.c \
      src/lista_ord.c \
      src/lista_gen.c \
      src/lista_cruz.c \
      src/estrutura.c \
      src/reader.c \
      src/main.c

OBJ = $(SRC:.c=.o)
TARGET = trabalho

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $(OBJ) $(LDFLAGS)

src/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET) resultado.csv

.PHONY: all clean
