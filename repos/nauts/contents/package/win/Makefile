ifeq (,$(POLYROOT))
	POLYROOT=../../Polycode
endif
ifeq (,$(PRODUCT))
	PRODUCT=Luanauts
endif
MINGW = /usr/local/i386-mingw32-4.3.0
LIBS = -lPolycode2DPhysics -lPolycodeLua -lPolycore -lfreetype -llibvorbisfile -llibvorbis -llibogg -lOpenAL32 -lphysfs -lpng -lzlib -lBox2D -llua5.1 -lwinmm -lws2_32 -lopengl32 -lglu32 -lmingw32
LDFLAGS = -L$(POLYROOT)/Framework/Core/lib -L$(POLYROOT)/Framework/Core/Dependencies/lib -L$(POLYROOT)/Framework/Modules/lib -L$(POLYROOT)/Framework/Modules/Dependencies/lib -L$(POLYROOT)/Framework/Bindings/Lua/Core/lib -L$(MINGW)/lib -mwindows
ROOT = ../..
SRC = $(ROOT)/source
TSRC = $(ROOT)/PolycodeTemplate
CXXFLAGS = -I$(POLYROOT)/Framework/Bindings/Lua/Modules/2DPhysics/include -I$(POLYROOT)/Framework/Bindings/Lua/Core/include -I$(POLYROOT)/Framework/Core/Dependencies/include/lua5.1 -I$(POLYROOT)/Framework/Core/include -I$(POLYROOT)/Framework/Core/Dependencies/include/AL -I$(POLYROOT)/Framework/Core/Dependencies/include/ -I$(POLYROOT)/Framework/Modules/Dependencies/include -I$(POLYROOT)/Framework/Modules/include -I$(ROOT)/lua/Include -I$(SRC) -I$(TSRC) -D_MINGW -D_WINDOWS -DUNICODE
CFLAGS = $(CXXFLAGS)
GCC = $(MINGW)/bin/i386-mingw32-g++

# TODO: Remove the need for this
ACTUAL_POLYCODE_SRC = $(ROOT)/../Polycode

BUILD = $(GCC) $(CXXFLAGS)
LINK = $(GCC) $(LDFLAGS)

all: $(PRODUCT)/$(PRODUCT)

$(PRODUCT)/$(PRODUCT): $(PRODUCT).exe
	mkdir -p $(PRODUCT)/Internal
	cp $(PRODUCT).exe $(PRODUCT)
	cp $(POLYROOT)/Framework/Core/Dependencies/bin/OpenAL32.dll $(PRODUCT)
	cp $(ROOT)/package/readme.txt $(PRODUCT)
	cp $(ROOT)/package/links.txt $(PRODUCT)
	cp $(POLYROOT)/Framework/Core/Assets/default.pak $(POLYROOT)/Framework/Bindings/Lua/Core/API/api.pak $(ROOT)/media.pak $(ROOT)/project.pak $(ROOT)/physics2d.pak $(PRODUCT)/Internal
	rm -f Luanauts_win.zip
	zip -r Luanauts_win.zip Luanauts

$(PRODUCT).exe: winmain.o PolycodeTemplateApp.o bridge.o cpVect.o program.o svgloader.o terminal.o ProjectLUA.o Physics2DLUA.o
	$(LINK) winmain.o PolycodeTemplateApp.o bridge.o cpVect.o program.o svgloader.o terminal.o ProjectLUA.o Physics2DLUA.o $(LIBS) -o $(PRODUCT).exe

winmain.o:
	$(BUILD) -c $(TSRC)/winmain.cpp

PolycodeTemplateApp.o:
	$(BUILD) -x c++ -c $(TSRC)/PolycodeTemplateApp.mm

bridge.o:
	$(BUILD) -c $(SRC)/bridge.cpp

cpVect.o:
	$(BUILD) -c $(SRC)/cpVect.cpp

program.o:
	$(BUILD) -c $(SRC)/program.cpp

svgloader.o:
	$(BUILD) -c $(SRC)/svgloader.cpp

terminal.o: ProjectLUA.o
	$(BUILD) -c $(SRC)/terminal.cpp

ProjectLUA.o:
	./make_lua.sh
	$(BUILD) -c $(ROOT)/lua/Source/ProjectLUA.cpp

Physics2DLUA.o:
	$(BUILD) -c $(ACTUAL_POLYCODE_SRC)/Modules/Bindings/2DPhysics/Source/Physics2DLUA.cpp

# I sure would like to put a rim riff $(PRODUCT) here, but what if
# someone accidentally sets PRODUCT=/ ?
clean:
	rm -f *.o *.exe 
	rm -f $(PRODUCT)/*.exe $(PRODUCT)/*.pak $(PRODUCT)/*.dll
