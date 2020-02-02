ifneq (,$(DEBUG))
	ETC = -g -DSELF_EDIT
endif
GCC = g++
AR = ar 

FTGL=../../iphone/Classes/ftgles/src
FTGL_INCLUDE = -I$(FTGL)/.. -I$(FTGL)/../include -I$(FTGL)/../include/Freetype -I$(FTGL)/../include/Freetype/config -I$(FTGL) -I$(FTGL)/FTGL

SRC = ..
INCLUDE = -I/usr/include/freetype2 -I../..
LIBS = -lfreetype -lGL -lGLU -lSDLmain -lSDL 
BUILD = $(GCC) $(ETC) -DLINUX -DNDEBUG -I. -I$(SRC) $(INCLUDE) -I../static_lib $(FTGL_INCLUDE)

all: pongpongpongpongpongpongpongpong.exe
	rm -rf pongpongpongpongpongpongpongpong
	mkdir pongpongpongpongpongpongpongpong
	cp pongpongpongpongpongpongpongpong.exe pongpongpongpongpongpongpongpong/pongpongpongpongpongpongpongpong
	cp -R ../Internal pongpongpongpongpongpongpongpong
	cp ../../atlas/output_image/* pongpongpongpongpongpongpongpong/Internal
	cp ../../media/* pongpongpongpongpongpongpongpong/Internal
	cp ../../shader/position.vsh ../../shader/color.fsh ../../shader/blurcolor.fsh pongpongpongpongpongpongpongpong/Internal 
	cp ../Readme.txt pongpongpongpongpongpongpongpong/Readme.txt
	cp ../Custom.txt pongpongpongpongpongpongpongpong/Custom.txt
	rm -rf pongpongpongpongpongpongpongpong/Internal/.svn
	rm -rf pongpongpongpongpongpongpongpong/Internal/.DS_Store

pongpongpongpongpongpongpongpong.exe: main.o program.o display.o gui.o util_display.o text_display.o glCommonMatrix.o glCommon.o controls.o slice.o color.o lodepng.o internalfile_common.o GLee.o tinyxml.a chipmunk.a ftgl.a
	$(BUILD) main.o program.o display.o gui.o util_display.o text_display.o glCommonMatrix.o glCommon.o controls.o slice.o color.o lodepng.o internalfile_common.o GLee.o tinyxml.a chipmunk.a ftgl.a $(LIBS) -o pongpongpongpongpongpongpongpong.exe

main.o:
	$(BUILD) -c $(SRC)/main.cpp

program.o:
	$(BUILD) -c $(SRC)/../program.cpp

display.o:
	$(BUILD) -c $(SRC)/../display.cpp

gui.o:
	$(BUILD) -c $(SRC)/gui.cpp

util_display.o: 
	$(BUILD) -c $(SRC)/../util_display.cpp

text_display.o:
	$(BUILD) -c $(SRC)/../text_display.cpp

glCommonMatrix.o:
	$(BUILD) -c $(SRC)/../glCommonMatrix.cpp

glCommon.o:
	$(BUILD) -c $(SRC)/../glCommon.cpp

controls.o:
	$(BUILD) -c $(SRC)/../controls.cpp
	
color.o:
	$(BUILD) -c $(SRC)/../color.cpp
	
lodepng.o:
	$(BUILD) -c $(SRC)/lodepng.cpp
	
slice.o:
	$(BUILD) -c $(SRC)/../slice.cpp

internalfile_common.o:
	$(BUILD) -c $(SRC)/internalfile_common.cpp
	
GLee.o:
	$(BUILD) -c ../GLee.c 
	
tinyxml.a:
	$(BUILD) -c ../static_lib/tinyxml.cpp
	$(BUILD) -c ../static_lib/tinyxmlerror.cpp
	$(BUILD) -c ../static_lib/tinyxmlparser.cpp
	$(AR) rvs tinyxml.a tinyxml.o tinyxmlerror.o tinyxmlparser.o
	
chipmunk.a:
	$(BUILD) -c ../static_lib/chipmunk.cpp
	$(BUILD) -c ../static_lib/cpArbiter.cpp
	$(BUILD) -c ../static_lib/cpArray.cpp
	$(BUILD) -c ../static_lib/cpBB.cpp
	$(BUILD) -c ../static_lib/cpBody.cpp
	$(BUILD) -c ../static_lib/cpCollision.cpp
	$(BUILD) -c ../static_lib/cpHashSet.cpp
	$(BUILD) -c ../static_lib/cpPolyShape.cpp
	$(BUILD) -c ../static_lib/cpShape.cpp
	$(BUILD) -c ../static_lib/cpSpace.cpp
	$(BUILD) -c ../static_lib/cpSpaceComponent.cpp
	$(BUILD) -c ../static_lib/cpSpaceHash.cpp
	$(BUILD) -c ../static_lib/cpSpaceQuery.cpp
	$(BUILD) -c ../static_lib/cpSpaceStep.cpp
	$(BUILD) -c ../static_lib/cpVect.cpp
	$(BUILD) -c ../static_lib/constraints/cpConstraint.cpp
	$(BUILD) -c ../static_lib/constraints/cpDampedRotarySpring.cpp
	$(BUILD) -c ../static_lib/constraints/cpDampedSpring.cpp
	$(BUILD) -c ../static_lib/constraints/cpGearJoint.cpp
	$(BUILD) -c ../static_lib/constraints/cpGrooveJoint.cpp
	$(BUILD) -c ../static_lib/constraints/cpPinJoint.cpp
	$(BUILD) -c ../static_lib/constraints/cpPivotJoint.cpp
	$(BUILD) -c ../static_lib/constraints/cpRatchetJoint.cpp
	$(BUILD) -c ../static_lib/constraints/cpRotaryLimitJoint.cpp
	$(BUILD) -c ../static_lib/constraints/cpSimpleMotor.cpp
	$(BUILD) -c ../static_lib/constraints/cpSlideJoint.cpp
	$(AR) rvs chipmunk.a chipmunk.o cpArbiter.o cpArray.o cpBB.o cpBody.o cpCollision.o cpHashSet.o cpPolyShape.o cpShape.o cpSpace.o cpSpaceComponent.o cpSpaceHash.o cpSpaceQuery.o cpSpaceStep.o cpVect.o cpConstraint.o cpDampedRotarySpring.o cpDampedSpring.o cpGearJoint.o cpGrooveJoint.o cpPinJoint.o cpPivotJoint.o cpRatchetJoint.o cpRotaryLimitJoint.o cpSimpleMotor.o cpSlideJoint.o

ftgl.a:
	$(BUILD) -c $(FTGL)/FTVectoriser.cpp
	$(BUILD) -c $(FTGL)/FTBuffer.cpp
	$(BUILD) -c $(FTGL)/FTCharmap.cpp
	$(BUILD) -c $(FTGL)/FTContour.cpp
	$(BUILD) -c $(FTGL)/FTFace.cpp
	$(BUILD) -c $(FTGL)/FTGlyphContainer.cpp
	$(BUILD) -c $(FTGL)/FTLibrary.cpp
	$(BUILD) -c $(FTGL)/FTPoint.cpp
	$(BUILD) -c $(FTGL)/FTSize.cpp
	$(BUILD) -c $(FTGL)/FTGlyph/FTPolygonGlyph.cpp
	$(BUILD) -c $(FTGL)/FTGlyph/FTOutlineGlyph.cpp
	$(BUILD) -c $(FTGL)/FTGlyph/FTTextureGlyph.cpp
	$(BUILD) -c $(FTGL)/FTGlyph/FTBitmapGlyph.cpp
	$(BUILD) -c $(FTGL)/FTGlyph/FTBufferGlyph.cpp
	$(BUILD) -c $(FTGL)/FTGlyph/FTGlyph.cpp
	$(BUILD) -c $(FTGL)/FTGlyph/FTGlyphGlue.cpp
	$(BUILD) -c $(FTGL)/FTLayout/FTLayout.cpp
	$(BUILD) -c $(FTGL)/FTLayout/FTLayoutGlue.cpp
	$(BUILD) -c $(FTGL)/FTLayout/FTSimpleLayout.cpp
	$(BUILD) -c $(FTGL)/FTFont/FTPolygonFont.cpp
	$(BUILD) -c $(FTGL)/FTFont/FTOutlineFont.cpp
	$(BUILD) -c $(FTGL)/FTFont/FTBitmapFont.cpp
	$(BUILD) -c $(FTGL)/FTFont/FTTextureFont.cpp
	$(BUILD) -c $(FTGL)/FTFont/FTBufferFont.cpp
	$(BUILD) -c $(FTGL)/FTFont/FTFont.cpp
	$(BUILD) -c $(FTGL)/FTFont/FTFontGlue.cpp
	$(BUILD) -c $(FTGL)/FTGL/ftglesGlue.cpp
	$(AR) rvs ftgl.a FTVectoriser.o FTBuffer.o FTCharmap.o FTContour.o FTFace.o FTGlyphContainer.o FTLibrary.o FTPoint.o FTSize.o FTPolygonGlyph.o FTOutlineGlyph.o FTTextureGlyph.o FTBitmapGlyph.o FTBufferGlyph.o FTGlyph.o FTGlyphGlue.o FTLayout.o FTLayoutGlue.o FTSimpleLayout.o FTPolygonFont.o FTOutlineFont.o FTBitmapFont.o FTTextureFont.o FTBufferFont.o FTFont.o FTFontGlue.o ftglesGlue.o

clean:
	rm -rf pongpongpongpongpongpongpongpong
	rm *.o chipmunk.a tinyxml.a ftgl.a
	rm pongpongpongpongpongpongpongpong.exe
