CC=gcc
CFLAGS=-O3 -ffast-math -flto -Wall -Wextra -pthread -Iinclude
LDFLAGS=-lSDL2 -lSDL2_image -lexif -lm

OBJ = src/main.o src/image_handling.o src/event_handler.o src/rendering.o

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $< $(LDFLAGS)

slideshow: $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -f src/*.o slideshow
