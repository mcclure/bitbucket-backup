CC=gcc
CFLAGS=-c -std=c99 -Wall -Werror -O3
LDFLAGS=
SOURCES=main.c error.c utils.c settings.c graphics.c level.c dot.c dotmatrix.c player.c sound.c zpipe.c game.c lighting.c dotgen.c
OBJECTS=$(SOURCES:%.c=obj/%.o)
LIBS=-lSDL -lSOIL -lGL -lfmodex -lz
EXECUTABLE=epacse

all: $(SOURCES) $(EXECUTABLE)
	
$(EXECUTABLE): $(OBJECTS) 
	$(CC) $(LDFLAGS) $(OBJECTS) $(LIBS) -o $@

obj/%.o : %.c
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -fv obj/*.o $(EXECUTABLE)
