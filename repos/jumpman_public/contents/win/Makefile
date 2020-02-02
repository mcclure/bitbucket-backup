GCC = /opt/mingw/bin/i386-mingw32msvc-g++
AR = /opt/mingw/bin/i386-mingw32msvc-ar
WINDRES = /opt/mingw/bin/i386-mingw32msvc-windres
SRC = ..
INCLUDE = /opt/mingw/include
LIBS = -L/opt/mingw/lib -lmingw32 -L/opt/mingw/i386-mingw32msvc/lib -lopengl32 -lglu32 -lSDLmain -lSDL -mwindows
BUILD = $(GCC) -DWINDOWS -I. -I$(SRC) -I$(INCLUDE) -I../static_lib -I./lib -I./lib/freetype2 -I../FTGL

all: Jumpman.exe
	rm -rf Jumpman
	mkdir Jumpman
	cp Jumpman.exe Jumpman
	cp Internal/mingwm10.dll Jumpman
	cp Internal/SDL.dll Jumpman
	cp -R ../Internal Jumpman
	cp Internal/icon.ico Jumpman/Internal
	cat ../Readme.txt | perl convert_line_breaks.pl > Jumpman/Readme.txt 
	rm -rf Jumpman/Internal/.svn
	rm -rf Jumpman/Internal/.DS_Store
	rm -rf Jumpman/Internal/Main.jmp/.svn
	rm -rf Jumpman/Internal/Main.jmp/.DS_Store

Jumpman.exe: icon.o main.o jumpman.o interface.o controls.o color.o lodepng.o internalfile_windows.o tinyxml.a chipmunk.a ftgl.a
	$(BUILD) icon.o main.o jumpman.o interface.o controls.o color.o lodepng.o internalfile_windows.o tinyxml.a chipmunk.a ftgl.a lib/libfreetype.a $(LIBS) -o Jumpman.exe

icon.o:
	$(WINDRES) -i icon.rc -o icon.o

main.o:
	$(BUILD) -c $(SRC)/main.cpp

jumpman.o:
	$(BUILD) -c $(SRC)/jumpman.cpp

controls.o:
	$(BUILD) -c $(SRC)/controls.cpp
	
interface.o:
	$(BUILD) -c $(SRC)/interface.cpp
	
color.o:
	$(BUILD) -c $(SRC)/color.cpp
	
lodepng.o:
	$(BUILD) -c $(SRC)/lodepng.cpp

internalfile_windows.o:
	$(BUILD) -c $(SRC)/internalfile_windows.cpp
	
tinyxml.a:
	$(BUILD) -c ../static_lib/tinyxml.cpp
	$(BUILD) -c ../static_lib/tinyxmlerror.cpp
	$(BUILD) -c ../static_lib/tinyxmlparser.cpp
	$(AR) rvs tinyxml.a tinyxml.o tinyxmlerror.o tinyxmlparser.o
	
chipmunk.a:
	$(BUILD) -c ../static_lib/chipmunk.c
	$(BUILD) -c ../static_lib/cpCollision.c
	$(BUILD) -c ../static_lib/cpSpace.c
	$(BUILD) -c ../static_lib/cpArbiter.c
	$(BUILD) -c ../static_lib/cpHashSet.c
	$(BUILD) -c ../static_lib/cpSpaceHash.c
	$(BUILD) -c ../static_lib/cpArray.c
	$(BUILD) -c ../static_lib/cpJoint.c
	$(BUILD) -c ../static_lib/cpVect.c
	$(BUILD) -c ../static_lib/cpBB.c
	$(BUILD) -c ../static_lib/cpPolyShape.c
	$(BUILD) -c ../static_lib/cpBody.c
	$(BUILD) -c ../static_lib/cpShape.c
	$(AR) rvs chipmunk.a chipmunk.o cpBB.o cpHashSet.o cpShape.o cpVect.o cpArbiter.o cpBody.o cpJoint.o cpSpace.o cpArray.o cpCollision.o cpPolyShape.o cpSpaceHash.o

ftgl.a:
	$(BUILD) -c ../ftgl/FTBitmapGlyph.cpp
	$(BUILD) -c ../ftgl/FTCharmap.cpp
	$(BUILD) -c ../ftgl/FTContour.cpp
#	$(BUILD) -c ../ftgl/FTExtrdGlyph.cpp
	$(BUILD) -c ../ftgl/FTFace.cpp
	$(BUILD) -c ../ftgl/FTFont.cpp
	$(BUILD) -c ../ftgl/FTGLBitmapFont.cpp
#	$(BUILD) -c ../ftgl/FTGLExtrdFont.cpp
#	$(BUILD) -c ../ftgl/FTGLOutlineFont.cpp
	$(BUILD) -c ../ftgl/FTGLPixmapFont.cpp
#	$(BUILD) -c ../ftgl/FTGLPolygonFont.cpp
	$(BUILD) -c ../ftgl/FTGLTextureFont.cpp
	$(BUILD) -c ../ftgl/FTGlyph.cpp
	$(BUILD) -c ../ftgl/FTGlyphContainer.cpp
	$(BUILD) -c ../ftgl/FTLibrary.cpp
#	$(BUILD) -c ../ftgl/FTOutlineGlyph.cpp
	$(BUILD) -c ../ftgl/FTPixmapGlyph.cpp
	$(BUILD) -c ../ftgl/FTPoint.cpp
#	$(BUILD) -c ../ftgl/FTPolyGlyph.cpp
	$(BUILD) -c ../ftgl/FTSize.cpp
	$(BUILD) -c ../ftgl/FTTextureGlyph.cpp
#	$(BUILD) -c ../ftgl/FTVectoriser.cpp
	$(AR) rvs ftgl.a FTBitmapGlyph.o FTCharmap.o FTContour.o FTFace.o FTFont.o FTGLBitmapFont.o FTGLPixmapFont.o FTGLTextureFont.o FTGlyph.o FTGlyphContainer.o FTLibrary.o FTPixmapGlyph.o FTPoint.o FTSize.o FTTextureGlyph.o

clean:
	rm -rf Jumpman
	rm *.o chipmunk.a tinyxml.a ftgl.a
	rm Jumpman.exe
