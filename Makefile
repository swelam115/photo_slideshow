CC=gcc
CFLAGS=-O3 -ffast-math -flto -Wall -Wextra -pthread -Iinclude
LDFLAGS=-lSDL2 -lSDL2_image -lm

OBJ = src/slideshow.o src/image_handling.o src/event_handler.o src/rendering.o

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

slideshow: $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -f *.o slideshow
