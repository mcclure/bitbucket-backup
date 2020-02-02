ifeq (,$(POLYROOT))
	POLYROOT=../../Polycode
endif
ifeq (,$(PRODUCT))
	PRODUCT=LD23
endif
LIBS = -lPolycode2DPhysics -lPolycode3DPhysics -lPolycodeLua -lPolycore -lfreetype -llibogg -llibvorbis -llibvorbisfile -lopenal -lphysfs -lpng -lz -lBox2D -lBulletDynamics -lBulletCollision -lBulletMultiThreaded -lBulletSoftBody -lBulletSoftBodySolvers_CPU -lBulletSoftBodySolvers_OpenCL_Mini -lLinearMath -lMiniCL -llua5.1 -lSDL -lGLU -lGL
LDFLAGS = -L$(POLYROOT)/Framework/Core/lib -L$(POLYROOT)/Framework/Core/Dependencies/lib -L$(POLYROOT)/Framework/Modules/lib -L$(POLYROOT)/Framework/Modules/Dependencies/lib -L$(POLYROOT)/Framework/Bindings/Lua/Core/lib '-Wl,-rpath,$$ORIGIN/Internal'
ROOT = ../..
SRC = $(ROOT)/source
TSRC = $(ROOT)/PolycodeTemplate
CXXFLAGS = -I$(POLYROOT)/Framework/Bindings/Lua/Modules/2DPhysics/include -I$(POLYROOT)/Framework/Bindings/Lua/Modules/3DPhysics/include -I$(POLYROOT)/Framework/Bindings/Lua/Core/include -I$(POLYROOT)/Framework/Core/Dependencies/include/lua5.1 -I$(POLYROOT)/Framework/Core/include -I$(POLYROOT)/Framework/Core/Dependencies/include/AL -I$(POLYROOT)/Framework/Core/Dependencies/include -I$(POLYROOT)/Framework/Modules/Dependencies/include -I$(POLYROOT)/Framework/Modules/Dependencies/include/bullet -I$(POLYROOT)/Framework/Modules/include -I$(ROOT)/lua/Include -I$(SRC) -I$(TSRC)
CFLAGS = $(CXXFLAGS)
GCC = g++ 

BUILD = $(GCC) $(CXXFLAGS)
LINK = $(GCC) $(LDFLAGS)

all: $(PRODUCT)/$(PRODUCT)

$(PRODUCT)/$(PRODUCT): $(PRODUCT).exe
	mkdir -p $(PRODUCT)/Internal
	cp $(POLYROOT)/Framework/Core/Dependencies/lib/libopenal.so.1 $(PRODUCT)/Internal
	cp $(PRODUCT).exe $(PRODUCT)/$(PRODUCT)
	cp $(ROOT)/package/readme.txt $(PRODUCT)
	cp $(POLYROOT)/Framework/Core/Assets/default.pak $(POLYROOT)/Framework/Bindings/Lua/Core/API/api.pak $(ROOT)/media.pak $(ROOT)/project.pak $(ROOT)/physics2d.pak $(ROOT)/physics3d.pak $(PRODUCT)/Internal
	rm -f LD23_lin.zip
	zip -r LD23_lin.zip LD23

$(PRODUCT).exe: main.o PolycodeTemplateApp.o bridge.o playtest.o dos.o cpVect.o program.o svgloader.o terminal.o ProjectLUA.o Physics2DLUA.o Physics3DLUA.o
	$(LINK) main.o PolycodeTemplateApp.o bridge.o playtest.o dos.o cpVect.o program.o svgloader.o terminal.o ProjectLUA.o Physics2DLUA.o Physics3DLUA.o $(LIBS) -o $(PRODUCT).exe

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

# I sure would like to put a rim riff $(PRODUCT) here, but what if
# someone accidentally sets PRODUCT=/ ?
clean:
	rm -f *.o *.exe 
	rm -f $(PRODUCT)/*.exe $(PRODUCT)/*.pak $(PRODUCT)/*.dll
