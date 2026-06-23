CC ?= cc
CFLAGS ?= -std=c11 -Wall -Wextra -Werror -O0 -g
CPPFLAGS ?= -Iinclude -D_POSIX_C_SOURCE=200809L

BIN := bin/jash
SRC := $(shell find src -name '*.c' -type f)
OBJ := $(SRC:.c=.o)

all: $(BIN)

$(BIN): $(OBJ) | bin
	$(CC) $(CFLAGS) $(OBJ) -o $@

%.o: %.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

bin:
	mkdir -p bin

clean:
	rm -f $(OBJ) $(BIN)

run: all
	./$(BIN)

.PHONY: all clean run bin
