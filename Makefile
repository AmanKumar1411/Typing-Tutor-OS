CC = cc
CFLAGS = -Wall -Wextra -Werror -std=c11 -Iinclude
SRC = src/main.c src/memory.c src/string_utils.c src/math_utils.c src/keyboard.c src/screen.c
BIN = typing_tutor

all: $(BIN)

$(BIN): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(BIN)

run: $(BIN)
	./$(BIN)

clean:
	rm -f $(BIN)

.PHONY: all run clean
