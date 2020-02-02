//-*****************************************************************************
//
// Copyright (c) 2009-2010, Industrial Light & Magic,
//   a division of Lucasfilm Entertainment Company Ltd.
//
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
// *       Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
// *       Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
// *       Neither the name of Industrial Light & Magic nor the names of
// its contributors may be used to endorse or promote products derived
// from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
//-*****************************************************************************

#ifndef __GL_CAMERA_H__
#define __GL_CAMERA_H__

#include "Foundation.h"

#include <ImathBox.h>
#include <ImathVec.h>
#include <ImathLine.h>
#include <ImathMatrix.h>


//-*****************************************************************************
class GLCamera
{
public:
    GLCamera();
    ~GLCamera() {};
    
    // Executes OpenGL commands to show the current camera
    void apply() const;


    //-*************************************************************************
    // Transform
    Imath::M44d transform() const;
    Imath::M44d projection() const;
    
    Imath::V2d project(const Imath::V3d worldPoint);
    Imath::Line3d unproject(const Imath::V2d cameraPoint);
    
    const Imath::V3d &rotation() const { return m_rotation; }
    void setRotation( const Imath::V3d &r ) { m_rotation = r; }

    const Imath::V3d &scale() const { return m_scale; }
    void setScale( const Imath::V3d &s ) { m_scale = s; }

    const Imath::V3d &translation() const { return m_translation; }
    void setTranslation( const Imath::V3d &t ) { m_translation = t; }

    double centerOfInterest() const { return m_centerOfInterest; }
    void setCenterOfInterest( double coi )
    {
        m_centerOfInterest = std::max( coi, 0.1 );
    }

    Imath::V3d pointOfInterest() const;

    double fovy() const { return m_fovy; }
    void setFovy( double fvy ) { m_fovy = fvy; }

    int width() const { return m_size.x; }
    int height() const { return m_size.y; }
    
    void autoSetClippingPlanes( const Imath::Box3d &bounds );
    void setClippingPlanes( double clipNear, double clipFar )
    {
        m_clip.x = clipNear;
        m_clip.y = clipFar;
    }


    //-*************************************************************************
    // UI Actions
    void track( const Imath::V2d &point );
    void dolly( const Imath::V2d &point, 
                double dollySpeed = 5.0 );
    void rotate( const Imath::V2d &point,
                 double rotateSpeed = 400.0 );

    void rotateAngle( const Imath::V2d &angle );

    void frame( const Imath::Box3d &bounds );

    void lookAt( const Imath::V3d &eye, const Imath::V3d &at );

    void setSize( int w, int h )
    {
        m_size.x = w;
        m_size.y = h;
        m_aspect = (double)w / (double)h;
    }
    void setSize( const Imath::V2i &sze )
    {
        m_size = sze;
        m_aspect = (double)sze.x / (double)sze.y;
    }


    //-*************************************************************************
    // RIB STUFF
    std::string RIB() const;


protected:
    //-*************************************************************************
    // DATA
    Imath::V3d m_rotation;
    Imath::V3d m_scale;
    Imath::V3d m_translation;
   
    double m_centerOfInterest;
    double m_fovy;
    Imath::V2d m_clip;
    Imath::V2i m_size;
    double m_aspect;
};


#endif  
