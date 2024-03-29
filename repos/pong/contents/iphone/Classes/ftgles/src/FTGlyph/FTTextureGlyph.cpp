/*
 * FTGL - OpenGL font library
 *
 * Copyright (c) 2001-2004 Henry Maddocks <ftgl@opengl.geek.nz>
 * Copyright (c) 2008 Sam Hocevar <sam@zoy.org>
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "ftgles_config.h"

#include <math.h>

#include "FTGL/ftgles.h"

#include "FTInternals.h"
#include "FTTextureGlyphImpl.h"


//
//  FTGLTextureGlyph
//


FTTextureGlyph::FTTextureGlyph(FT_GlyphSlot glyph, int id, int xOffset,
                               int yOffset, int width, int height) :
    FTGlyph(new FTTextureGlyphImpl(glyph, id, xOffset, yOffset, width, height))
{}


FTTextureGlyph::~FTTextureGlyph()
{}


const FTPoint& FTTextureGlyph::Render(const FTPoint& pen, int renderMode)
{
#ifndef TARGET_ANDROID
    FTTextureGlyphImpl *myimpl = dynamic_cast<FTTextureGlyphImpl *>(impl);
#else
    // No RTTI for you!
    FTTextureGlyphImpl *myimpl = (FTTextureGlyphImpl *)(impl);
#endif
    return myimpl->RenderImpl(pen, renderMode);
}


//
//  FTGLTextureGlyphImpl
//


GLint FTTextureGlyphImpl::activeTextureID = 0;
vector<char> growableTemp;

FTTextureGlyphImpl::FTTextureGlyphImpl(FT_GlyphSlot glyph, int id, int xOffset,
                                       int yOffset, int width, int height)
:   FTGlyphImpl(glyph),
    destWidth(0),
    destHeight(0),
    glTextureID(id)
{
    /* FIXME: need to propagate the render mode all the way down to
     * here in order to get FT_RENDER_MODE_MONO aliased fonts.
     */

    err = FT_Render_Glyph(glyph, FT_RENDER_MODE_NORMAL);
    if(err || glyph->format != ft_glyph_format_bitmap)
    {
        return;
    }

    FT_Bitmap      bitmap = glyph->bitmap;

    destWidth  = bitmap.width;
    destHeight = bitmap.rows;

    if(destWidth && destHeight)
    {
       // glPushClientAttrib(GL_CLIENT_PIXEL_STORE_BIT);
        
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glBindTexture(GL_TEXTURE_2D, glTextureID);
#if JUMPCORE_32BIT_TEXTURES
        int totalMemory = destWidth*destHeight*JUMPCORE_TEXTURE_SIZE;
        growableTemp.resize(totalMemory);
        for(int c =0; c < totalMemory; c++) {
            growableTemp[c] = (3==c%4)?bitmap.buffer[c/4]:0xFF; // RGB = 0xFF A = 0x00. Is this safe on PPC endian systems?
        }        
        glTexSubImage2D(GL_TEXTURE_2D, 0, xOffset, yOffset, destWidth, destHeight, GL_RGBA, GL_UNSIGNED_BYTE, &growableTemp[0]);
#else
        glTexSubImage2D(GL_TEXTURE_2D, 0, xOffset, yOffset, destWidth, destHeight, GL_ALPHA, GL_UNSIGNED_BYTE, bitmap.buffer);
#endif

       // glPopClientAttrib();
    }


//      0
//      +----+
//      |    |
//      |    |
//      |    |
//      +----+
//           1

    uv[0].X(static_cast<float>(xOffset) / static_cast<float>(width));
    uv[0].Y(static_cast<float>(yOffset) / static_cast<float>(height));
    uv[1].X(static_cast<float>(xOffset + destWidth) / static_cast<float>(width));
    uv[1].Y(static_cast<float>(yOffset + destHeight) / static_cast<float>(height));

    corner = FTPoint(glyph->bitmap_left, glyph->bitmap_top);
}


FTTextureGlyphImpl::~FTTextureGlyphImpl()
{}


const FTPoint& FTTextureGlyphImpl::RenderImpl(const FTPoint& pen,
                                              int renderMode)
{
    float dx, dy;
    GLfloat colors[4];
    
    if(activeTextureID != glTextureID)
    {
        glBindTexture(GL_TEXTURE_2D, (GLuint)glTextureID);
        activeTextureID = glTextureID;
    }
    
    dx = floor(pen.Xf() + corner.Xf());
    dy = floor(pen.Yf() + corner.Yf());
    
//    glGetFloatv(GL_CURRENT_COLOR, colors); // FIXME: Disabled for Jumpcore, but should be added back in w/glCommon support
    
    ftglBegin(GL_QUADS);
    
//    ftglColor4f(colors[0], colors[1], colors[2], colors[3]);
    
    ftglTexCoord2f(uv[0].Xf(), uv[0].Yf());
    ftglVertex2f(dx, dy);
    
    ftglTexCoord2f(uv[0].Xf(), uv[1].Yf());
    ftglVertex2f(dx, dy - destHeight);
    
    ftglTexCoord2f(uv[1].Xf(), uv[1].Yf());
    ftglVertex2f(dx + destWidth, dy - destHeight);
    
    ftglTexCoord2f(uv[1].Xf(), uv[0].Yf());
    ftglVertex2f(dx + destWidth, dy);
    
    ftglEnd();
    
    return advance;
}

