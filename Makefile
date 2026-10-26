CC = gcc
CFLAGS = -Wall -Wextra -pedantic -std=c99 -O2 -Ic_project/include -D_DEFAULT_SOURCE -D_POSIX_C_SOURCE=200809L
LDFLAGS = -lm

SRC = c_project/src/generic_item.c \
      c_project/src/generic_search.c \
      c_project/src/fila.c \
      c_project/src/pilha.c \
      c_project/src/lista_seq.c \
      c_project/src/lista_enc.c \
      c_project/src/lista_cabeca.c \
      c_project/src/lista_ord.c \
      c_project/src/lista_gen.c \
      c_project/src/lista_cruz.c \
      c_project/src/estrutura.c \
      c_project/src/reader.c \
      c_project/src/main.c

OBJ = $(SRC:.c=.o)
TARGET = trabalho

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $(OBJ) $(LDFLAGS)

src/%.o: c_project/src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET) resultado.csv

.PHONY: all clean
