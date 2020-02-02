ifneq (,$(DEBUG))
	ETC = -g -DSELF_EDIT
else
	ETC = -DNDEBUG
endif
GCC = g++
AR = ar 

PRODUCT=Jumpcore

FTGL=../../iphone/Classes/ftgles/src
FTGL_INCLUDE = -I$(FTGL)/.. -I$(FTGL)/../include -I$(FTGL)/../include/Freetype -I$(FTGL)/../include/Freetype/config -I$(FTGL) -I$(FTGL)/FTGL

SRC = ..
INCLUDE = -I/usr/include/freetype2 -I../..
LIBS = -lfreetype -lGL -lGLU -lSDL2main -lSDL2 -lportaudio
BUILD = $(GCC) $(ETC) -DLINUX -I. -I$(SRC) $(INCLUDE) -I../glm_lib -I../plaid -I../static_lib $(FTGL_INCLUDE)

all: $(PRODUCT).exe
	rm -rf $(PRODUCT)
	mkdir $(PRODUCT)
	cp $(PRODUCT).exe $(PRODUCT)/$(PRODUCT) 
	cp -R ../Internal $(PRODUCT) 
	cp ../../atlas/output_image/* $(PRODUCT)/Internal
	cp ../../shader/* $(PRODUCT)/Internal
	cp ../Readme.txt $(PRODUCT)/Readme.txt 
	rm -rf $(PRODUCT)/Internal/.svn
	rm -rf $(PRODUCT)/Internal/.DS_Store
	zip -r $(PRODUCT)_lin.zip $(PRODUCT)

$(PRODUCT).exe: main.o program.o display.o ent.o display_ent.o test_ent.o chipmunk_ent.o input_ent.o postprocess.o pile.o util_pile.o input.o plaidext.o util_thread.o freeze.o util_display.o text_display.o glCommonMatrix.o glCommon.o controls.o slice.o color.o stb_image.o stb_image_write.o internalfile_common.o GLee.o game.o basement.o tinyxml.a chipmunk.a ftgl.a pg_audioimp_portaudio.o plaid.a
	$(BUILD) main.o program.o display.o util_display.o text_display.o ent.o display_ent.o test_ent.o chipmunk_ent.o input_ent.o postprocess.o pile.o util_pile.o input.o plaidext.o util_thread.o freeze.o glCommonMatrix.o glCommon.o controls.o slice.o color.o stb_image.o stb_image_write.o internalfile_common.o GLee.o tinyxml.a game.o basement.o chipmunk.a ftgl.a pg_audioimp_portaudio.o plaid.a $(LIBS) -o $(PRODUCT).exe

# Jumpcore

main.o:
	$(BUILD) -c $(SRC)/main.cpp

program.o:
	$(BUILD) -c $(SRC)/../program.cpp

display.o:
	$(BUILD) -c $(SRC)/../display.cpp

ent.o:
	$(BUILD) -c $(SRC)/ent.cpp

display_ent.o:
	$(BUILD) -c $(SRC)/display_ent.cpp

test_ent.o:
	$(BUILD) -c $(SRC)/test_ent.cpp

chipmunk_ent.o:
	$(BUILD) -c $(SRC)/chipmunk_ent.cpp

input_ent.o:
	$(BUILD) -c $(SRC)/input_ent.cpp

postprocess.o:
	$(BUILD) -c $(SRC)/postprocess.cpp

util_pile.o:
	$(BUILD) -c $(SRC)/util_pile.cpp

pile.o:
	$(BUILD) -c $(SRC)/pile.cpp

input.o:
	$(BUILD) -c $(SRC)/input.cpp

plaidext.o:
	$(BUILD) -c $(SRC)/plaidext.cpp

util_thread.o:
	$(BUILD) -c $(SRC)/util_thread.cpp

freeze.o:
	$(BUILD) -c $(SRC)/freeze.cpp

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
	
stb_image.o:
	$(BUILD) -c $(SRC)/stb_image.cpp

stb_image_write.o:
	$(BUILD) -c $(SRC)/stb_image_write.cpp

slice.o:
	$(BUILD) -c $(SRC)/../slice.cpp
	
internalfile_common.o:
	$(BUILD) -c $(SRC)/internalfile_common.cpp
	
GLee.o:
	$(BUILD) -c ../GLee.c

pg_audioimp_portaudio.o: # Impl file cannot go into plaid.a, because of the static-time registration hack.
	$(BUILD) -c ../plaid/imp-portaudio/pg_audioimp_portaudio.cpp

# Project-specific

game.o:
	$(BUILD) -c $(SRC)/game.cpp
	
basement.o:
	$(BUILD) -c $(SRC)/basement.cpp

# Libs
	
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

plaid.a:
	$(BUILD) -c ../plaid/codec_stb/pg_codec_ogg_stb.cpp
	$(BUILD) -c ../plaid/plaid/audio/analysis/envelope.cpp
	$(BUILD) -c ../plaid/plaid/audio/audio.cpp
	$(BUILD) -c ../plaid/plaid/audio/chunk.cpp
	$(BUILD) -c ../plaid/plaid/audio/clip.cpp
	$(BUILD) -c ../plaid/plaid/audio/codec.cpp
	$(BUILD) -c ../plaid/plaid/audio/driver.cpp
	$(BUILD) -c ../plaid/plaid/audio/effect/amp.cpp
	$(BUILD) -c ../plaid/plaid/audio/effect/filter.cpp
	$(BUILD) -c ../plaid/plaid/audio/effect/pan.cpp
	$(BUILD) -c ../plaid/plaid/audio/effect/pitch.cpp
	$(BUILD) -c ../plaid/plaid/audio/effect/reverb.cpp
	$(BUILD) -c ../plaid/plaid/audio/scheduler.cpp
	$(BUILD) -c ../plaid/plaid/audio/scratch.cpp
	$(BUILD) -c ../plaid/plaid/audio/signal.cpp
	$(BUILD) -c ../plaid/plaid/audio/synth/oscillator.cpp
	$(BUILD) -c ../plaid/plaid/audio/util/mixer.cpp
	$(BUILD) -c ../plaid/plaid/audio/util/splicer.cpp
	$(BUILD) -c ../plaid/plaid/audio/util/splitter.cpp
	$(BUILD) -c ../plaid/plaid/audio/util/transcoder.cpp
	$(BUILD) -c ../plaid/plaid/util/binary.cpp
	$(BUILD) -c ../plaid/plaid/util/memory.cpp
	$(AR) rvs plaid.a pg_codec_ogg_stb.o envelope.o audio.o chunk.o clip.o codec.o driver.o amp.o filter.o pan.o pitch.o reverb.o scheduler.o scratch.o signal.o oscillator.o mixer.o splicer.o splitter.o transcoder.o binary.o memory.o

clean:
	rm -f $(PRODUCT)/*.exe $(PRODUCT)/*.dll
	rm -rf $(PRODUCT)/Internal
	rm -f *.o chipmunk.a tinyxml.a ftgl.a plaid.a
	rm -f $(PRODUCT).exe
	rm -f $(PRODUCT)_win.zip
