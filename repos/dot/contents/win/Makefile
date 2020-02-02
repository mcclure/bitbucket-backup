ifneq (,$(DEBUG))
	ETC = -g -DSELF_EDIT
endif
ifeq (,$(WINDOWS))
	ifeq (,$(MINGW))
		MINGW=/usr/local/i386-mingw32-4.3.0
	endif
	GCC = $(MINGW)/bin/i386-mingw32-g++
	AR = $(MINGW)/bin/i386-mingw32-ar
	WINDRES = $(MINGW)/bin/i386-mingw32-windres
	INCLUDE = -I$(MINGW)/include
	# LIBDIR = -L$(MINGW)/lib     # You'd *think* this line would be needed?
else
	GCC = g++
	AR = ar
	WINDRES = windres
	INCLUDE = -I/usr/include
	LIBDIR = -L/usr/lib
endif
SRC = ..
LIBS = $(LIBDIR) -lmingw32 -lopengl32 -lglu32 -lSDLmain -lSDL -mwindows
BUILD = $(GCC) $(ETC) -DWINDOWS -I. -I$(SRC) $(INCLUDE) -I../static_lib -I./lib -I./lib/freetype2 -I../FTGL

all: Jumpcore.exe
	rm -rf Jumpcore
	mkdir Jumpcore
	cp Jumpcore.exe Jumpcore
	cp Internal/mingwm10.dll Jumpcore
	cp Internal/SDL.dll Jumpcore
	cp -R ../Internal Jumpcore
	cp Internal/icon.ico Jumpcore/Internal
ifeq (,$(WINDOWS))
		cat ../Readme.txt | perl convert_line_breaks.pl > Jumpcore/Readme.txt 
endif
	rm -rf Jumpcore/Internal/.svn
	rm -rf Jumpcore/Internal/.DS_Store

Jumpcore.exe: icon.o main.o program.o display.o slice.o color.o lodepng.o internalfile_windows.o tinyxml.a ftgl.a
	$(BUILD) icon.o main.o program.o display.o slice.o color.o lodepng.o internalfile_windows.o tinyxml.a ftgl.a lib/libfreetype.a $(LIBS) -o Jumpcore.exe

icon.o:
	$(WINDRES) -i icon.rc -o icon.o

main.o:
	$(BUILD) -c $(SRC)/main.cpp

program.o:
	$(BUILD) -c $(SRC)/program.cpp

display.o:
	$(BUILD) -c $(SRC)/display.cpp

controls.o:
	$(BUILD) -c $(SRC)/controls.cpp
	
color.o:
	$(BUILD) -c $(SRC)/color.cpp
	
lodepng.o:
	$(BUILD) -c $(SRC)/lodepng.cpp

slice.o:
	$(BUILD) -c $(SRC)/slice.cpp

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
	rm -rf Jumpcore
	rm *.o tinyxml.a ftgl.a
	rm Jumpcore.exe
