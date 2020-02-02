# Copyright (C) 2009 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

JNI_DIR := $(call my-dir)
LOCAL_PATH := $(JNI_DIR)
include $(CLEAR_VARS)
include $(JNI_DIR)/freetype/Android.mk

LOCAL_PATH := $(JNI_DIR)
include $(CLEAR_VARS)

SD := ../../..
SDP := $(LOCAL_PATH)/$(SD)

SLD = $(SD)/desktop/static_lib
SFD = $(SD)/iphone/Classes/ftgles/src
SFDP = $(LOCAL_PATH)/$(SFD)

FTGL_INCLUDE = -I$(SFDP)/.. -I$(SFDP)/FTGL -I$(SFDP)/../include -I$(SFDP)/../include/Freetype -I$(SFDP)/../include/Freetype/config -I$(SFDP)/iGLU-1.0.0/include
FTGL_SRC = $(SFD)/FTVectoriser.cpp $(SFD)/FTBuffer.cpp $(SFD)/FTCharmap.cpp $(SFD)/FTContour.cpp $(SFD)/FTFace.cpp $(SFD)/FTGlyphContainer.cpp $(SFD)/FTLibrary.cpp $(SFD)/FTPoint.cpp $(SFD)/FTSize.cpp $(SFD)/iGLU-1.0.0/libutil/error.c $(SFD)/iGLU-1.0.0/libutil/glue.c $(SFD)/iGLU-1.0.0/libutil/project.c $(SFD)/iGLU-1.0.0/libutil/registry.c $(SFD)/iGLU-1.0.0/libtess/dict.c $(SFD)/iGLU-1.0.0/libtess/geom.c $(SFD)/iGLU-1.0.0/libtess/memalloc.c $(SFD)/iGLU-1.0.0/libtess/mesh.c $(SFD)/iGLU-1.0.0/libtess/normal.c $(SFD)/iGLU-1.0.0/libtess/priorityq.c $(SFD)/iGLU-1.0.0/libtess/render.c $(SFD)/iGLU-1.0.0/libtess/sweep.c $(SFD)/iGLU-1.0.0/libtess/tess.c $(SFD)/iGLU-1.0.0/libtess/tessmono.c $(SFD)/FTGlyph/FTPolygonGlyph.cpp $(SFD)/FTGlyph/FTOutlineGlyph.cpp $(SFD)/FTGlyph/FTTextureGlyph.cpp $(SFD)/FTGlyph/FTBitmapGlyph.cpp $(SFD)/FTGlyph/FTBufferGlyph.cpp $(SFD)/FTGlyph/FTGlyph.cpp $(SFD)/FTGlyph/FTGlyphGlue.cpp $(SFD)/FTLayout/FTLayout.cpp $(SFD)/FTLayout/FTLayoutGlue.cpp $(SFD)/FTLayout/FTSimpleLayout.cpp $(SFD)/FTFont/FTPolygonFont.cpp $(SFD)/FTFont/FTOutlineFont.cpp $(SFD)/FTFont/FTBitmapFont.cpp $(SFD)/FTFont/FTTextureFont.cpp $(SFD)/FTFont/FTBufferFont.cpp $(SFD)/FTFont/FTFont.cpp $(SFD)/FTFont/FTFontGlue.cpp $(SFD)/FTGL/ftglesGlue.cpp
CHIPMUNK_SRC =  $(SLD)/chipmunk.cpp $(SLD)/cpArbiter.cpp $(SLD)/cpArray.cpp $(SLD)/cpBB.cpp $(SLD)/cpBody.cpp $(SLD)/cpCollision.cpp $(SLD)/cpHashSet.cpp $(SLD)/cpPolyShape.cpp $(SLD)/cpShape.cpp $(SLD)/cpSpace.cpp $(SLD)/cpSpaceComponent.cpp $(SLD)/cpSpaceHash.cpp $(SLD)/cpSpaceQuery.cpp $(SLD)/cpSpaceStep.cpp $(SLD)/cpVect.cpp $(SLD)/constraints/cpConstraint.cpp $(SLD)/constraints/cpDampedRotarySpring.cpp $(SLD)/constraints/cpDampedSpring.cpp $(SLD)/constraints/cpGearJoint.cpp $(SLD)/constraints/cpGrooveJoint.cpp $(SLD)/constraints/cpPinJoint.cpp $(SLD)/constraints/cpPivotJoint.cpp $(SLD)/constraints/cpRatchetJoint.cpp $(SLD)/constraints/cpRotaryLimitJoint.cpp $(SLD)/constraints/cpSimpleMotor.cpp $(SLD)/constraints/cpSlideJoint.cpp
JUMPCORE_SRC = $(SD)/util_display.cpp $(SD)/glCommon.cpp $(SD)/text_display.cpp $(SD)/glCommonMatrix.cpp $(SD)/controls.cpp $(SD)/color.cpp $(SD)/desktop/stb_image.cpp $(SD)/slice.cpp $(SD)/desktop/static_lib/tinyxml.cpp $(SD)/desktop/static_lib/tinyxmlerror.cpp $(SD)/desktop/static_lib/tinyxmlparser.cpp
PROGRAM_SRC = $(SD)/program.cpp $(SD)/display.cpp

# FIXME: Move desktop/ stuff which is actually shared as appropriate
# FIXME: Make SELF_EDIT dependent on NDK_DEBUG

#DEBUG MODE: Uncomment this and set android:debuggable in the manifest to true
# DEBUGMODE_FLAGS := -DSELF_EDIT
#RELEASE MODE: Comment the above line, uncomment below, android:debuggable false
DEBUGMODE_FLAGS := -DNDEBUG

LOCAL_MODULE    := native-activity
LOCAL_CFLAGS    := -Werror -DTARGET_ANDROID -I$(SDP) -I$(SDP)/desktop -I$(SDP)/desktop/static_lib -I$(SFDP) $(FTGL_INCLUDE) $(DEBUGMODE_FLAGS)
LOCAL_SRC_FILES := gl_code.cpp $(PROGRAM_SRC) $(JUMPCORE_SRC) $(CHIPMUNK_SRC) $(FTGL_SRC)
LOCAL_LDLIBS    := -llog -lGLESv1_CM -lGLESv2 -landroid -lEGL -lOpenSLES
LOCAL_STATIC_LIBRARIES := ft2 android_native_app_glue 

include $(BUILD_SHARED_LIBRARY)

$(call import-module,android/native_app_glue) 
