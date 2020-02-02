ifeq (,$(POLYROOT))
	POLYROOT=../../Polycode
endif
ifeq (,$(PRODUCT))
	PRODUCT=Polyethylene
endif
LIBS = -lPolycode2DPhysics -lPolycode3DPhysics -lPolycodeUI -lPolycodeLua -lPolycore -lfreetype -llibogg -llibvorbis -llibvorbisfile -lphysfs -lpng -lz -lBox2D -lBulletDynamics -lBulletCollision -lBulletMultiThreaded -lBulletSoftBody -lBulletSoftBodySolvers_OpenCL_Mini -lLinearMath -lMiniCL -llua5.1 -lSDL -lGLU -lGL
LDFLAGS = -L$(POLYROOT)/Framework/Core/lib -L$(POLYROOT)/Framework/Core/Dependencies/lib -L$(POLYROOT)/Framework/Modules/lib -L$(POLYROOT)/Framework/Modules/Dependencies/lib -L$(POLYROOT)/Framework/Bindings/Lua/Core/lib
ROOT = ../..
SRC = $(ROOT)/source
TSRC = $(ROOT)/PolycodeTemplate
CXXFLAGS = -I$(POLYROOT)/Framework/Bindings/Lua/Modules/2DPhysics/include -I$(POLYROOT)/Framework/Bindings/Lua/Modules/3DPhysics/include -I$(POLYROOT)/Framework/Bindings/Lua/Modules/UI/include -I$(POLYROOT)/Framework/Bindings/Lua/Core/include -I$(POLYROOT)/Framework/Core/Dependencies/include/lua5.1 -I$(POLYROOT)/Framework/Core/include -I$(POLYROOT)/Framework/Core/Dependencies/include/AL -I$(POLYROOT)/Framework/Core/Dependencies/include -I$(POLYROOT)/Framework/Modules/Dependencies/include -I$(POLYROOT)/Framework/Modules/Dependencies/include/bullet -I$(POLYROOT)/Framework/Modules/include -I$(ROOT)/lua/Include -I$(SRC) -I$(TSRC)
CFLAGS = $(CXXFLAGS)
ifeq (,$(DEBUG))
GCC = g++
else
GCC = g++ -g -D_DEBUG=1
endif

BUILD = $(GCC) $(CXXFLAGS)
LINK = $(GCC) $(LDFLAGS)

all: $(PRODUCT)/$(PRODUCT)

$(PRODUCT)/$(PRODUCT): $(PRODUCT).exe
	mkdir -p $(PRODUCT)/Internal
	cp $(PRODUCT).exe $(PRODUCT)/$(PRODUCT)
	cp $(ROOT)/package/readme.txt $(PRODUCT)
	cp $(POLYROOT)/Framework/Core/Assets/default.pak $(POLYROOT)/Framework/Bindings/Lua/Core/API/api.pak $(ROOT)/media.pak $(POLYROOT)/Framework/Extra/UIThemes.pak $(ROOT)/project.pak $(ROOT)/physics2d.pak $(ROOT)/physics3d.pak $(ROOT)/UI.pak $(PRODUCT)/Internal
	rm -f Polyethylene_lin.zip
	zip -r Polyethylene_lin.zip Polyethylene

$(PRODUCT).exe: main.o PolycodeTemplateApp.o bridge.o playtest.o dos.o cpVect.o program.o svgloader.o terminal.o ProjectLUA.o Physics2DLUA.o Physics3DLUA.o UILUA.o
	$(LINK) main.o PolycodeTemplateApp.o bridge.o playtest.o dos.o cpVect.o program.o svgloader.o terminal.o ProjectLUA.o Physics2DLUA.o Physics3DLUA.o UILUA.o $(LIBS) -o $(PRODUCT).exe

main.o:
	$(BUILD) -c $(TSRC)/main.cpp

PolycodeTemplateApp.o:
	$(BUILD) -x c++ -c $(TSRC)/PolycodeTemplateApp.mm

bridge.o:
	$(BUILD) -c $(SRC)/bridge.cpp
	
playtest.o:
	$(BUILD) -c $(SRC)/playtest.cpp

dos.o:
	$(BUILD) -c $(SRC)/dos.cpp

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

UILUA.o:
	$(BUILD) -c $(POLYROOT)/Framework/Extra/UILUA.cpp

# I sure would like to put a rim riff $(PRODUCT) here, but what if
# someone accidentally sets PRODUCT=/ ?
clean:
	rm -f *.o *.exe 
	rm -f $(PRODUCT)/*.exe $(PRODUCT)/*.pak $(PRODUCT)/*.dll
