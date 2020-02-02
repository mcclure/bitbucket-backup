ifeq (,$(POLYROOT))
	POLYROOT=/Users/mcc/work/gh/Polycode/Core/Winroot
endif
ifeq (,$(PRODUCT))
	PRODUCT=Template
endif
MINGW = /usr/local/i386-mingw32-4.3.0
LIBS = -lPolyCore -lphysfs -lpng -lz -lfreetype -lSDL -lopenal -lopengl32 -lglu32 -lmingw32
LDFLAGS = -L$(POLYROOT)/lib -L$(MINGW)/lib -mwindows
SRC = ../Xcode\ Template/PolycodeTemplate
CXXFLAGS = -I$(POLYROOT)/include -D_MINGW -I$(SRC)/../support -I$(MINGW)/include -I$(POLYROOT)/include/freetype2 -I$(SRC) -DSELF_EDIT
CFLAGS = $(CXXFLAGS)
CPPFLAGS = $(CXXFLAGS)
GCC = $(MINGW)/bin/i386-mingw32-g++ 
CLUTTER = $(POLYROOT)/share/polycore

BUILD = $(GCC) $(CPPFLAGS)
LINK = $(GCC) $(LDFLAGS)

all: $(PRODUCT)/$(PRODUCT).exe 

$(PRODUCT)/$(PRODUCT).exe: $(PRODUCT).exe
	mkdir -p $(PRODUCT)
	cp $(PRODUCT).exe ../Xcode\ Template/media.pak ../../Core/Assets/default.pak $(CLUTTER)/mingwm10.dll $(CLUTTER)/physfs.dll $(CLUTTER)/SDL.dll $(CLUTTER)/OpenAL32.dll $(CLUTTER)/msvcr100.dll $(PRODUCT)

$(PRODUCT).exe: PolycodeTemplateApp.o Program.o main.o
	$(LINK) PolycodeTemplateApp.o Program.o main.o $(LIBS) -o $(PRODUCT).exe

main.o:
	$(BUILD) -c main.cpp

Program.o:
	$(BUILD) -c $(SRC)/Program.cpp

PolycodeTemplateApp.o:
	$(BUILD) -c $(SRC)/PolycodeTemplateApp.mm

cpVect.o:
	$(BUILD) -c $(SRC)/../support/cpVect.cpp

# I sure would like to put a rim riff $(PRODUCT) here, but what if
# someone accidentally sets PRODUCT=/ ?
clean:
	rm -f *.o *.exe 
	rm -f $(PRODUCT)/*.exe $(PRODUCT)/*.pak $(PRODUCT)/*.dll
