//-*****************************************************************************
//
// Copyright (c) 2009-2010,
//  Sony Pictures Imageworks, Inc. and
//  Industrial Light & Magic, a division of Lucasfilm Entertainment Company Ltd.
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
// *       Neither the name of Sony Pictures Imageworks, nor
// Industrial Light & Magic nor the names of their contributors may be used
// to endorse or promote products derived from this software without specific
// prior written permission.
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

#include "GLCamera.h"

#include <cstdio>

#include <qgl.h>

/*
#if defined(__APPLE__) && defined(__MACH__)
#  include <OpenGL/gl.h>
#  include <OpenGL/glu.h>
#else
#  include <GL/gl.h>
#  include <GL/glu.h>
#endif
*/

#include <assert.h>

#include <vector>

using namespace Imath;

//-*****************************************************************************
GLCamera::GLCamera()
  : m_rotation( 0.0, 0.0, 0.0 ),
    m_scale( 1.0, 1.0, 1.0 ),
    m_translation( 0.0, 0.0, 0.0 ),
	m_centerOfInterest( 15.0 ),
    m_fovy( 45.0 ),
    m_clip( 0.01, 1000 ),
    m_size( 100, 100 ),
    m_aspect( 1.0 )
{
    // Nothing
}

//-*****************************************************************************
static inline void rotateVector( double rx, double ry, V3d &v )
{
    rx = radians( rx );
    const double sinX = sinf( rx );
    const double cosX = cosf( rx );

    const V3d t( v.x,
                 ( v.y * cosX ) - ( v.z * sinX ),
                 ( v.y * sinX ) + ( v.z * cosX ) );

    ry = radians( ry );
    const double sinY = sinf( ry );
    const double cosY = cosf( ry );

    v.x = ( t.x * cosY ) + ( t.z * sinY );
    v.y = t.y;
    v.z = ( t.x * -sinY ) + ( t.z * cosY );
}

//-*****************************************************************************
void GLCamera::frame( const Box3d &bounds )
{
    // Center the camera properly
    const V3d dt = m_translation - pointOfInterest();
    const V3d look = bounds.center();
    const V3d newTrans = look + dt;

    lookAt(newTrans, look);


    // Translate back to see the object
    Imath::V3d checkPoints[8];
    checkPoints[0] = Imath::V3d(bounds.min.x, bounds.min.y, bounds.min.z);
    checkPoints[1] = Imath::V3d(bounds.max.x, bounds.min.y, bounds.min.z);
    checkPoints[2] = Imath::V3d(bounds.min.x, bounds.max.y, bounds.min.z);
    checkPoints[3] = Imath::V3d(bounds.min.x, bounds.min.y, bounds.max.z);
    checkPoints[4] = Imath::V3d(bounds.min.x, bounds.max.y, bounds.max.z);
    checkPoints[5] = Imath::V3d(bounds.max.x, bounds.min.y, bounds.max.z);
    checkPoints[6] = Imath::V3d(bounds.max.x, bounds.max.y, bounds.min.z);
    checkPoints[7] = Imath::V3d(bounds.max.x, bounds.max.y, bounds.max.z);

    // Decide which fov is smaller and use that
    const double fovx = m_fovy * m_aspect;
    double usefulFOV = fovx;
    if (m_fovy < fovx) usefulFOV = m_fovy;

    // Figure out which of the 8 bbox extents is the furthest away and use that.
    Imath::V3d xn(0,0,0);
    for (int i = 0; i < 8; i++)
    {
        const Imath::Line3d lookVec(look, m_translation);
        const Imath::V3d closestPoint = lookVec.closestPointTo(checkPoints[i]);
        const double distance = (closestPoint - checkPoints[i]).length();
        const double newDistance = distance / tan(radians(usefulFOV) / 2.0);
        const double dDiff = (look - m_translation).length() - (closestPoint - m_translation).length();
        const Imath::V3d finalVec = (lookVec.dir * dDiff) + (lookVec.dir * newDistance);
        if (finalVec.length() > xn.length())
            xn = finalVec;
    }

    lookAt(look+xn, look);
}

//-*****************************************************************************
Imath::V3d GLCamera::pointOfInterest() const
{ 
    Imath::V3d v(0.0, 0.0, -m_centerOfInterest);
    rotateVector(m_rotation.x, m_rotation.y, v);
    return m_translation + v;
}

//-*****************************************************************************
void GLCamera::autoSetClippingPlanes( const Box3d &bounds )
{
    const double rotX = m_rotation.x;
    const double rotY = m_rotation.y;
    const V3d &eye = m_translation;
    double clipNear = FLT_MAX;
    double clipFar = FLT_MIN;
	
    V3d v( 0.0, 0.0, -m_centerOfInterest );
    rotateVector( rotX, rotY, v );
    v.normalize();
    
    V3d points[8];
    
    points[0] = V3d( bounds.min.x, bounds.min.y, bounds.min.z );
    points[1] = V3d( bounds.min.x, bounds.min.y, bounds.max.z );
    points[2] = V3d( bounds.min.x, bounds.max.y, bounds.min.z );
    points[3] = V3d( bounds.min.x, bounds.max.y, bounds.max.z );
    points[4] = V3d( bounds.max.x, bounds.min.y, bounds.min.z );
    points[5] = V3d( bounds.max.x, bounds.min.y, bounds.max.z );
    points[6] = V3d( bounds.max.x, bounds.max.y, bounds.min.z );
    points[7] = V3d( bounds.max.x, bounds.max.y, bounds.max.z );
    
    for( int p = 0; p < 8; ++p )
    {
        V3d dp = points[p] - eye;
        double proj = dp.dot( v );
        clipNear = std::min( proj, clipNear );
        clipFar = std::max( proj, clipFar );
    }
    
    clipNear -= 0.5f;
    clipFar += 0.5f;
    clipNear = clamp( clipNear, 0.1, 100000.0 );
    clipFar = clamp( clipFar, 0.1, 100000.0 );
    
    assert( clipFar > clipNear );
    
    m_clip[0] = clipNear;
    m_clip[1] = clipFar;
}

//-*****************************************************************************
void GLCamera::lookAt( const V3d &eye, const V3d &at )
{
    // TODO: Fix when the world is upside-down
    m_translation = eye;

    const V3d dt = at - eye;
	
    const double xzLen = sqrt( ( dt.x * dt.x ) +
                               ( dt.z * dt.z ) );

    m_rotation.x =  degrees( atan2( dt.y, xzLen ) );
    m_rotation.y = -degrees( atan2( dt.x, -dt.z ) );    
    
    m_centerOfInterest = dt.length();
}

//-*****************************************************************************
void GLCamera::apply() const
{
    glViewport( 0, 0, ( GLsizei )m_size[0], ( GLsizei )m_size[1] );
    glMatrixMode( GL_PROJECTION );
    glLoadIdentity();
    gluPerspective( m_fovy, 
                    ( ( GLdouble )m_size[0] ) /
                    ( ( GLdouble )m_size[1] ), 
                    m_clip[0], 
                    m_clip[1] );

    glMatrixMode( GL_MODELVIEW );
    glLoadIdentity();    

    ::glScaled( 1.0 / m_scale[0], 1.0 / m_scale[1], 1.0 / m_scale[2] );
    ::glRotated( -m_rotation[2], 0.0, 0.0, 1.0 );
    ::glRotated( -m_rotation[0], 1.0, 0.0, 0.0 );
    ::glRotated( -m_rotation[1], 0.0, 1.0, 0.0 );
    ::glTranslated( -m_translation[0], -m_translation[1], -m_translation[2] );
}

//-*****************************************************************************
M44d GLCamera::transform() const
{
    M44d m;
    M44d tmp;
    m.makeIdentity();

    tmp.setScale( V3d( 1.0 / m_scale[0],
                       1.0 / m_scale[1], 
                       1.0 / m_scale[2] )  );
    m = m * tmp;
    
    tmp.setAxisAngle( V3d( 0.0, 0.0, 1.0 ), radians( m_rotation[2] ) );
    m = m * tmp;
    tmp.setAxisAngle( V3d( 1.0, 0.0, 0.0 ), radians( m_rotation[0] ) );
    m = m * tmp;
    tmp.setAxisAngle( V3d( 0.0, 1.0, 0.0 ), radians( m_rotation[1] ) );
    m = m * tmp;
    
    tmp.setTranslation( V3d( m_translation[0],
                             m_translation[1],     
                             m_translation[2] ) ); 
    m = m * tmp;
    
    return m;
}

//-*****************************************************************************
M44d GLCamera::projection() const
{
    const double zFar = m_clip[1];
    const double zNear = m_clip[0];

    double rads = radians(m_fovy) / 2.0;
    const double deltaZ = zFar - zNear;
    const double sine = sin(rads);
    if ((deltaZ == 0) || (sine == 0) || (m_aspect == 0))
        return M44d();
    const double cotangent = cos(rads) / sine;

    M44d m;
    m[0][0] = cotangent / m_aspect;
    m[1][1] = cotangent;
    m[2][2] = -(zFar + zNear) / deltaZ;
    m[2][3] = -1;
    m[3][2] = -2 * zNear * zFar / deltaZ;
    m[3][3] = 0;

    return m;
}

//-*****************************************************************************
Imath::V2d GLCamera::project(const Imath::V3d worldPoint)
{
    Imath::V3d pInCam;
    transform().inverse().multVecMatrix(worldPoint, pInCam);

    Imath::V3d proj;
    projection().multVecMatrix(pInCam, proj);
    proj = (proj * 0.5) + Imath::V3d(0.5, 0.5, 0.5);
    proj.x = proj.x * (double)width() + 0;             // TODO: Viewport offset
    proj.y = proj.y * (double)height() + 0;

    return Imath::V2d(proj.x, proj.y);
}

//-*****************************************************************************
Imath::Line3d GLCamera::unproject(const Imath::V2d cameraPoint)
{
    Imath::V3d cPoint(cameraPoint.x, cameraPoint.y, -1.0);
    cPoint.x = (cPoint.x - 0.0) / (double)width();     // TODO: Viewport offset
    cPoint.y = (cPoint.y - 0.0) / (double)height();
    
    cPoint.x = (cPoint.x * 2.0) - 1.0;
    cPoint.y = (cPoint.y * 2.0) - 1.0;
    
    Imath::V3d result1;
    projection().inverse().multVecMatrix(cPoint, result1);

    Imath::V3d result2;
    transform().multVecMatrix(result1, result2);

    return Imath::Line3d(transform().translation(), result2);
}

//-*****************************************************************************
void GLCamera::track( const V2d &point )
{
    // INIT
    const double rotX = m_rotation.x;
    const double rotY = m_rotation.y;

    V3d dS( 1.0, 0.0, 0.0 );
    rotateVector( rotX, rotY, dS );

    V3d dT( 0.0, 1.0, 0.0 );
    rotateVector( rotX, rotY, dT );

    double multS = 2.0 * m_centerOfInterest * tanf( radians( fovy() ) / 2.0 );
    const double multT = multS / double( height() );
    multS /= double( width() );

    // TRACK
    const double s = -multS * point.x;
    const double t = multT * point.y;

    // ALTER
    setTranslation( ( m_translation +
                      ( s * dS ) + ( t * dT ) ) );
}

//-*****************************************************************************
void GLCamera::dolly( const V2d &point,
                      double dollySpeed )
{
    // INIT
    const double rotX = m_rotation.x;
    const double rotY = m_rotation.y;
    const V3d &eye = m_translation;
	
    V3d v( 0.0, 0.0, -m_centerOfInterest );
    rotateVector( rotX, rotY, v );
    const V3d view = eye + v;
    v.normalize();

    // DOLLY
    const double t = point.x / double( width() );

    // Magic dolly function
    double dollyBy = 1.0 - expf( -dollySpeed * t );

    //assert( fabsf( dollyBy ) < 1.0 );
    dollyBy *= m_centerOfInterest;
    const V3d newEye = eye + ( dollyBy * v );

    // ALTER
    setTranslation( newEye );
    v = newEye - view;
    m_centerOfInterest = v.length();
}

//-*****************************************************************************
void GLCamera::rotate( const V2d &point,
                       double rotateSpeed )
{
    // INIT
    double rotX = m_rotation.x;
    double rotY = m_rotation.y;
    const double rotZ = m_rotation.z;
    V3d eye = m_translation;

    V3d v( 0.0, 0.0, -m_centerOfInterest );
    rotateVector( rotX, rotY, v );

    const V3d view = eye + v;

    // ROTATE
    rotY += -rotateSpeed * ( point.x / double( width() ) );
    rotX += -rotateSpeed * ( point.y / double( height() ) );

    v[0] = 0.0;
    v[1] = 0.0;
    v[2] = m_centerOfInterest;
    rotateVector( rotX, rotY, v );

    // ALTER
    setTranslation( view + v );
    setRotation( V3d( rotX, rotY, rotZ ) );
}

//-*****************************************************************************
void GLCamera::rotateAngle( const V2d &angle )
{
    // INIT
    V3d eye = m_translation;
    double rotX = m_rotation.x;
    double rotY = m_rotation.y;
    const double rotZ = m_rotation.z;

    V3d v( 0.0, 0.0, -m_centerOfInterest );
    rotateVector( rotX, rotY, v );

    const V3d view = eye + v;

    // ROTATE
    rotY += angle[0];
    rotX += angle[1];

    v[0] = 0.0;
    v[1] = 0.0;
    v[2] = m_centerOfInterest;
    rotateVector( rotX, rotY, v );

    // ALTER
    setTranslation( view + v );
    setRotation( V3d( rotX, rotY, rotZ ) );
}

//-*****************************************************************************
std::string GLCamera::RIB() const
{
    char str[1024];
    sprintf (str, "Format %d %d 1\n"
                  "Clipping %f %f\n"
                  "Projection \"perspective\" \"fov\" [%f]\n"
                  "Scale 1 1 -1\n"
                  "Scale %f %f %f\n"
                  "Rotate %f 0 0 1\n"
                  "Rotate %f 1 0 0\n"
                  "Rotate %f 0 1 0\n"
                  "Translate %f %f %f\n",
                  ( int )m_size[0], ( int )m_size[1],
                  ( float )m_clip[0], ( float )m_clip[1],
                  ( float )m_fovy,
                  ( float )( 1.0/m_scale[0] ),
                  ( float )( 1.0/m_scale[1] ),
                  ( float )( 1.0/m_scale[2] ),
                  ( float )( -m_rotation[2] ),
                  ( float )( -m_rotation[0] ),
                  ( float )( -m_rotation[1] ),
                  ( float )( -m_translation[0] ),
                  ( float )( -m_translation[1] ),
                  ( float )( -m_translation[2] ));
    
    // Then transpose and print.
    return ( std::string( str ) );
}


