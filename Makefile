CC = clang
CFLAGS = -Wall -Wextra -std=c11 $(shell sdl2-config --cflags)
LIBS = $(shell sdl2-config --libs) -lSDL2_ttf

TARGET = tetris
SRC = main.c src/game.c src/renderer.c src/input.c

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET) $(LIBS)

clean:
	rm -f $(TARGET)
