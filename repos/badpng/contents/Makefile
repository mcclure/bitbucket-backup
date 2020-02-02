CXX ?= g++
ifneq (,$(DEBUG))
CXXFLAGS:=$(CXXFLAGS) -g
endif
BUILD = $(CXX) $(CXXFLAGS)

all: main.cpp stb_image_write_bad.cpp stb_image.cpp
	$(BUILD) -c main.cpp
	$(BUILD) -c stb_image_write_bad.cpp
	$(BUILD) -c stb_image.cpp
	$(BUILD) -o badpng main.o stb_image_write_bad.o stb_image.o
	
clean:
	rm -f badpng *.o