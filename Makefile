CC=gcc
CFLAGS=-O3 -ffast-math -flto -Wall -Wextra -pthread
LDFLAGS=-lSDL2 -lSDL2_image -lm

OBJ = main.o image_handling.o event_handler.o rendering.o

%.o: %.c
    $(CC) $(CFLAGS) -c -o $@ $<

slideshow: $(OBJ)
    $(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
    rm -f *.o slideshow