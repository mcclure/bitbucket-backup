ifeq (,$(POLYROOT))
	POLYROOT=../../Polycode
endif
ifeq (,$(PRODUCT))
	PRODUCT=Flipper
endif
MINGW = /usr/local/i386-mingw32-4.3.0
LIBS = -lPolycode2DPhysics -lPolycode3DPhysics -lPolycodeLua -lPolycore -lfreetype -llibvorbisfile -llibvorbis -llibogg -lOpenAL32 -lphysfs -lpng -lzlib -lBox2D -lBulletDynamics -lBulletCollision -lBulletMultiThreaded -lBulletSoftBody -lBulletSoftBodySolvers_CPU -lBulletSoftBodySolvers_OpenCL_Mini -lLinearMath -lMiniCL -llua5.1 -lwinmm -lws2_32 -lopengl32 -lglu32 -lmingw32
LDFLAGS = -L$(POLYROOT)/Framework/Core/lib -L$(POLYROOT)/Framework/Core/Dependencies/lib -L$(POLYROOT)/Framework/Modules/lib -L$(POLYROOT)/Framework/Modules/Dependencies/lib -L$(POLYROOT)/Framework/Bindings/Lua/Core/lib -L$(MINGW)/lib -mwindows
ROOT = ../..
SRC = $(ROOT)/source
TSRC = $(ROOT)/PolycodeTemplate
CXXFLAGS = -I$(POLYROOT)/Framework/Bindings/Lua/Modules/2DPhysics/include -I$(POLYROOT)/Framework/Bindings/Lua/Modules/3DPhysics/include -I$(POLYROOT)/Framework/Bindings/Lua/Core/include -I$(POLYROOT)/Framework/Core/Dependencies/include/lua5.1 -I$(POLYROOT)/Framework/Core/include -I$(POLYROOT)/Framework/Core/Dependencies/include/AL -I$(POLYROOT)/Framework/Core/Dependencies/include -I$(POLYROOT)/Framework/Modules/Dependencies/include -I$(POLYROOT)/Framework/Modules/Dependencies/include/bullet -I$(POLYROOT)/Framework/Modules/include -I$(ROOT)/lua/Include -I$(SRC) -I$(TSRC) -D_MINGW -D_WINDOWS -DUNICODE
CFLAGS = $(CXXFLAGS)
GCC = $(MINGW)/bin/i386-mingw32-g++
WINDRES = $(MINGW)/bin/i386-mingw32-windres

BUILD = $(GCC) $(CXXFLAGS)
LINK = $(GCC) $(LDFLAGS)

all: $(PRODUCT)/$(PRODUCT)

$(PRODUCT)/$(PRODUCT): $(PRODUCT).exe
	mkdir -p $(PRODUCT)/Internal
	cp $(PRODUCT).exe $(PRODUCT)
	# Copy prebuilt OpenAL if prebuilt present
	test -d ../../../OpenAL-product && cp ../../../OpenAL-product/wrap_oal.dll ../../../OpenAL-product/OpenAL32.dll $(PRODUCT)
	# Copy Polycode OpenAL if prebuilt not present
	test -d ../../../OpenAL-product || cp $(POLYROOT)/Framework/Core/Dependencies/bin/OpenAL32.dll $(PRODUCT)
	cp $(ROOT)/package/readme.txt $(PRODUCT)
	cp $(POLYROOT)/Framework/Core/Assets/default.pak $(POLYROOT)/Framework/Bindings/Lua/Core/API/api.pak $(ROOT)/media.pak $(ROOT)/project.pak $(ROOT)/physics2d.pak $(ROOT)/physics3d.pak $(PRODUCT)/Internal
	rm -f Flipper_win.zip
	zip -r Flipper_win.zip Flipper

$(PRODUCT).exe: icon.o winmain.o PolycodeTemplateApp.o bridge.o playtest.o dos.o cpVect.o program.o cuber.o vox.o lodepng.o svgloader.o terminal.o ProjectLUA.o Physics2DLUA.o Physics3DLUA.o
	$(LINK) icon.o winmain.o PolycodeTemplateApp.o bridge.o playtest.o dos.o cpVect.o program.o cuber.o vox.o lodepng.o svgloader.o terminal.o ProjectLUA.o Physics2DLUA.o Physics3DLUA.o $(LIBS) -o $(PRODUCT).exe

icon.o:
	$(WINDRES) -i icon.rc -o icon.o

winmain.o:
	$(BUILD) -c $(TSRC)/winmain.cpp

PolycodeTemplateApp.o:
	$(BUILD) -x c++ -c $(TSRC)/PolycodeTemplateApp.mm

bridge.o:
	$(BUILD) -c $(SRC)/bridge.cpp
	
playtest.o:
	$(BUILD) -c $(SRC)/playtest.cpp

dos.o:
	$(BUILD) -c $(SRC)/dos.cpp

vox.o:
	$(BUILD) -c $(SRC)/vox.cpp

cuber.o:
	$(BUILD) -c $(SRC)/cuber.cpp

lodepng.o:
	$(BUILD) -c $(SRC)/lodepng.cpp

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
	$(BUILD) -c $(POLYROOT)/Framework/Extra/Physics2DLUA.cpp

Physics3DLUA.o:
	$(BUILD) -c $(POLYROOT)/Framework/Extra/Physics3DLUA.cpp

# I sure would like to put a rim riff $(PRODUCT) here, but what if
# someone accidentally sets PRODUCT=/ ?
clean:
	rm -f *.o *.exe 
	rm -f $(PRODUCT)/*.exe $(PRODUCT)/*.pak $(PRODUCT)/*.dll
