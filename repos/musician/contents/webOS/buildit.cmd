@echo off
@rem Set the device you want to build for to 1
set PRE=0
set PIXI=1
set DEBUG=0

@rem Name your output executable
del jumpcore.exe
set OUTFILE=jumpcore.exe

if %PRE% equ 0 if %PIXI% equ 0 goto :END

if %DEBUG% equ 1 (
   set DEVICEOPTS=-g
) else (
   set DEVICEOPTS=
)

if %PRE% equ 1 (
   set DEVICEOPTS=%DEVICEOPTS% -mcpu=cortex-a8 -mfpu=neon -mfloat-abi=softfp
)

if %PIXI% equ 1 (
   set DEVICEOPTS=%DEVICEOPTS% -mcpu=arm1136jf-s -mfpu=vfp -mfloat-abi=softfp
)

echo %DEVICEOPTS%

@rem List the libraries needed
@rem FreeType is not standard on webOS, you must include freetype.so in your project
set LIBS=-lSDL -lpdl -lGLESv2 -lSDL_image -lSDL_ttf -lfreetype

@rem list the includes for the project
set IPHONE_GLU=../iphone/Classes/ftgles/src/iGLU-1.0.0
set INCLUDE= -I../desktop/freetype_lib/freetype2 -I../ -I../desktop -I../desktop/static_lib "-I%IPHONE_GLU%/include"

@rem FreeType include
set FTGL=../iphone/Classes/ftgles/src
set FTGL_INCLUDE=-I%FTGL%/.. -I%FTGL%/../include -I%FTGL%/../include/Freetype -I%FTGL%/../include/Freetype/config -I%FTGL% -I%FTGL%/FTGL

@rem Build Command
set BUILD=arm-none-linux-gnueabi-g++ %DEVICEOPTS% -DTARGET_WEBOS -DSELF_EDIT -Wno-deprecated "-I%PALMPDK%\include" "-I%PALMPDK%\include\SDL" %INCLUDE% %FTGL_INCLUDE%
set AR=arm-none-linux-gnueabi-ar

@rem List your source files here
set SRC_FOLDER=.

echo main.o
%BUILD% -c %SRC_FOLDER%/main.cpp

echo program.o
%BUILD% -c %SRC_FOLDER%/../program.cpp

echo display.o
%BUILD% -c %SRC_FOLDER%/../display.cpp

echo util_display.o
%BUILD% -c %SRC_FOLDER%/../util_display.cpp

echo text_display.o
%BUILD% -c %SRC_FOLDER%/../text_display.cpp

echo glCommonMatrix.o
%BUILD% -c %SRC_FOLDER%/../glCommonMatrix.cpp

echo glCommon.o
%BUILD% -c %SRC_FOLDER%/../glCommon.cpp

echo controls.o
%BUILD% -c %SRC_FOLDER%/../controls.cpp

echo color.o
%BUILD% -c %SRC_FOLDER%/../color.cpp

echo stb_image.o
%BUILD% -c %SRC_FOLDER%/../desktop/stb_image.cpp

echo slice.o
%BUILD% -c %SRC_FOLDER%/../slice.cpp

echo tinyxml.a
%BUILD% -c ../desktop/static_lib/tinyxml.cpp
%BUILD% -c ../desktop/static_lib/tinyxmlerror.cpp
%BUILD% -c ../desktop/static_lib/tinyxmlparser.cpp
%AR% rvs tinyxml.a tinyxml.o tinyxmlerror.o tinyxmlparser.o

echo chipmunk.a
%BUILD% -c ../desktop/static_lib/chipmunk.cpp
%BUILD% -c ../desktop/static_lib/cpArbiter.cpp
%BUILD% -c ../desktop/static_lib/cpArray.cpp
%BUILD% -c ../desktop/static_lib/cpBB.cpp
%BUILD% -c ../desktop/static_lib/cpBody.cpp
%BUILD% -c ../desktop/static_lib/cpCollision.cpp
%BUILD% -c ../desktop/static_lib/cpHashSet.cpp
%BUILD% -c ../desktop/static_lib/cpPolyShape.cpp
%BUILD% -c ../desktop/static_lib/cpShape.cpp
%BUILD% -c ../desktop/static_lib/cpSpace.cpp
%BUILD% -c ../desktop/static_lib/cpSpaceComponent.cpp
%BUILD% -c ../desktop/static_lib/cpSpaceHash.cpp
%BUILD% -c ../desktop/static_lib/cpSpaceQuery.cpp
%BUILD% -c ../desktop/static_lib/cpSpaceStep.cpp
%BUILD% -c ../desktop/static_lib/cpVect.cpp
%BUILD% -c ../desktop/static_lib/constraints/cpConstraint.cpp
%BUILD% -c ../desktop/static_lib/constraints/cpDampedRotarySpring.cpp
%BUILD% -c ../desktop/static_lib/constraints/cpDampedSpring.cpp
%BUILD% -c ../desktop/static_lib/constraints/cpGearJoint.cpp
%BUILD% -c ../desktop/static_lib/constraints/cpGrooveJoint.cpp
%BUILD% -c ../desktop/static_lib/constraints/cpPinJoint.cpp
%BUILD% -c ../desktop/static_lib/constraints/cpPivotJoint.cpp
%BUILD% -c ../desktop/static_lib/constraints/cpRatchetJoint.cpp
%BUILD% -c ../desktop/static_lib/constraints/cpRotaryLimitJoint.cpp
%BUILD% -c ../desktop/static_lib/constraints/cpSimpleMotor.cpp
%BUILD% -c ../desktop/static_lib/constraints/cpSlideJoint.cpp
%AR% rvs chipmunk.a chipmunk.o cpArbiter.o cpArray.o cpBB.o cpBody.o cpCollision.o cpHashSet.o cpPolyShape.o cpShape.o cpSpace.o cpSpaceComponent.o cpSpaceHash.o cpSpaceQuery.o cpSpaceStep.o cpVect.o cpConstraint.o cpDampedRotarySpring.o cpDampedSpring.o cpGearJoint.o cpGrooveJoint.o cpPinJoint.o cpPivotJoint.o cpRatchetJoint.o cpRotaryLimitJoint.o cpSimpleMotor.o cpSlideJoint.o

echo ftgl.a
%BUILD% -c %FTGL%/FTVectoriser.cpp
%BUILD% -c %FTGL%/FTBuffer.cpp
%BUILD% -c %FTGL%/FTCharmap.cpp
%BUILD% -c %FTGL%/FTContour.cpp
%BUILD% -c %FTGL%/FTFace.cpp
%BUILD% -c %FTGL%/FTGlyphContainer.cpp
%BUILD% -c %FTGL%/FTLibrary.cpp
%BUILD% -c %FTGL%/FTPoint.cpp
%BUILD% -c %FTGL%/FTSize.cpp
%BUILD% -c %FTGL%/FTGlyph/FTPolygonGlyph.cpp
%BUILD% -c %FTGL%/FTGlyph/FTOutlineGlyph.cpp
%BUILD% -c %FTGL%/FTGlyph/FTTextureGlyph.cpp
%BUILD% -c %FTGL%/FTGlyph/FTBitmapGlyph.cpp
%BUILD% -c %FTGL%/FTGlyph/FTBufferGlyph.cpp
%BUILD% -c %FTGL%/FTGlyph/FTGlyph.cpp
%BUILD% -c %FTGL%/FTGlyph/FTGlyphGlue.cpp
%BUILD% -c %FTGL%/FTLayout/FTLayout.cpp
%BUILD% -c %FTGL%/FTLayout/FTLayoutGlue.cpp
%BUILD% -c %FTGL%/FTLayout/FTSimpleLayout.cpp
%BUILD% -c %FTGL%/FTFont/FTPolygonFont.cpp
%BUILD% -c %FTGL%/FTFont/FTOutlineFont.cpp
%BUILD% -c %FTGL%/FTFont/FTBitmapFont.cpp
%BUILD% -c %FTGL%/FTFont/FTTextureFont.cpp
%BUILD% -c %FTGL%/FTFont/FTBufferFont.cpp
%BUILD% -c %FTGL%/FTFont/FTFont.cpp
%BUILD% -c %FTGL%/FTFont/FTFontGlue.cpp
%BUILD% -c %FTGL%/FTGL/ftglesGlue.cpp
%AR% rvs ftgl.a FTVectoriser.o FTBuffer.o FTCharmap.o FTContour.o FTFace.o FTGlyphContainer.o FTLibrary.o FTPoint.o FTSize.o FTPolygonGlyph.o FTOutlineGlyph.o FTTextureGlyph.o FTBitmapGlyph.o FTBufferGlyph.o FTGlyph.o FTGlyphGlue.o FTLayout.o FTLayoutGlue.o FTSimpleLayout.o FTPolygonFont.o FTOutlineFont.o FTBitmapFont.o FTTextureFont.o FTBufferFont.o FTFont.o FTFontGlue.o ftglesGlue.o

@rem echo glue.a
@rem %BUILD% -c %IPHONE_GLU%/libutil/error.c
@rem %BUILD% -c %IPHONE_GLU%/libutil/glue.c
@rem %BUILD% -c %IPHONE_GLU%/libutil/project.c
@rem %BUILD% -c %IPHONE_GLU%/libutil/registry.c
@rem %AR% rvs glue.a error.o glue.o project.o registry.o

echo tess.a
%BUILD% -c %IPHONE_GLU%/libtess/dict.c
%BUILD% -c %IPHONE_GLU%/libtess/geom.c
%BUILD% -c %IPHONE_GLU%/libtess/memalloc.c
%BUILD% -c %IPHONE_GLU%/libtess/mesh.c
%BUILD% -c %IPHONE_GLU%/libtess/normal.c
%BUILD% -c %IPHONE_GLU%/libtess/priorityq-heap.c
%BUILD% -c %IPHONE_GLU%/libtess/priorityq.c
%BUILD% -c %IPHONE_GLU%/libtess/render.c
%BUILD% -c %IPHONE_GLU%/libtess/sweep.c
%BUILD% -c %IPHONE_GLU%/libtess/tess.c
%BUILD% -c %IPHONE_GLU%/libtess/tessmono.c
%AR% rvs tess.a dict.o geom.o memalloc.o mesh.o normal.o priorityq-heap.o priorityq.o render.o sweep.o tess.o tessmono.o

%BUILD% -o %OUTFILE% main.o program.o display.o util_display.o text_display.o glCommonMatrix.o glCommon.o controls.o stb_image.o slice.o color.o tinyxml.a chipmunk.a ftgl.a tess.a "-L." "-L%PALMPDK%\device\lib" -Wl,--allow-shlib-undefined %LIBS%

@rem copy files for package
rmdir /S /Q Jumpcore
mkdir Jumpcore
copy Jumpcore.exe .\Jumpcore
mkdir Jumpcore\Internal
copy ..\desktop\Internal .\Jumpcore\Internal
copy ..\atlas\output_image\* .\Jumpcore\Internal
copy ..\shader\position.vsh .\Jumpcore\Internal
copy ..\shader\color.fsh .\Jumpcore\Internal 
copy ..\desktop\Readme.txt .\Jumpcore
del .\Jumpcore\Internal\.svn
del .\Jumpcore\Internal\.DS_Store
@rem We need the current version of libstdc++
copy "%PALMPDK%\device\lib\libstdc++.so.6.0.9" ".\Jumpcore\libstdc++.so"

@rem clean out the temporary files
del *.o
del *.a

goto :EOF

:END
echo Please select the target device by editing the PRE/PIXI variable in this file.
exit /b 1

