TARGET = sword_bin

OFILES = ../ai.o \
	 ../buf.o \
	 ../chooser.o \
	 ../config.o \
	 ../civstate.o \
	 ../dircontrol.o \
	 ../display.o \
	 ../dpdf.o \
	 ../engine.o \
	 ../firefly.o \
	 ../gfxengine.o \
	 ../glbdef.o \
	 ../grammar.o \
	 ../item.o \
	 ../main.o \
	 ../map.o \
	 ../mapdata.o \
	 ../mob.o \
	 ../msg.o \
	 ../panel.o \
	 ../rand.o \
	 ../scrpos.o \
	 ../speed.o \
	 ../strageticview.o \
	 ../strategy.o \
	 ../target.o \
	 ../text.o \
	 ../thread.o \
	 ../thread_linux.o

.cpp.o:
	g++ -O3 -DLINUX -I../lib/libtcod-1.5.0/include `sdl-config --cflags` -c $< -o $@

all: $(TARGET)

$(TARGET): $(OFILES)
	g++ -O3 -o $(TARGET) $(OFILES) -L../lib/libtcod-1.5.0 `sdl-config --libs` -ltcod -ltcodxx -lSDL_mixer

clean:
	rm -f ../*.o
	rm -f $(TARGET)

premake:
	cd .. ; ./support/enummaker/enummaker source.txt
