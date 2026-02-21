CC = clang
CFLAGS = -Wall -Wextra -std=c11

TARGET = tetris
SRC = main.c

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET)

clean:
	rm -f $(TARGET)
