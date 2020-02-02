// OpenGL ES wrapper code -- part 2 (see comment)

// File contains code from Jumpcore; notice applies to that code only:
/* Copyright (C) 2008-2010 Andi McClure
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

#include "kludge.h"
#include "math.h"
#include "glCommonMatrix.h"

// So what's happening here?
//
// gl2 doesn't have the matrix operations from ES1; you are supposed to calculate your
// own matrix and then upload it. This is actually a better idea than using the matrix
// operations in the first place, but it kind of screws you over if you need to retain
// backward compatibility with existing ES1 code.
//
// My solution: Write a series of wrapper functions that call the ES1 matrix operation
// functions in ES1 mode, and in gl2 mode emulate the ES1 matrix operations on the CPU
// and then upload the results. Because I am not sure I could write the emulation code
// correctly myself, I did the second bit there by just copying and pasting huge blobs
// of code out of Mesa, the open source OpenGL implementation. The resulting code is a
// bit more heavyweight and verbose than it needs to be, because it ducks through Mesa
// abstraction layers designed to do things like give each thread its own context, but
// for now I'm assuming it's worth it because (1) the less I change, the less likely I
// am to introduce bugs and (2) it makes it easier to import more Mesa code in future.
//
// There are some problems:
// - I blanked out the "inverse matrix" functionality; I don't think it's used here
// - My ALIGN_MALLOC macro is not aligned. Why did they need it aligned?
// - Nothing allocated is ever freed!
//
// Note: In order for these methods, mesa_sync must be called. This is only done for
// jcBegin...jcEnd blocks as described in ftglesGlue().

/* START MESA 7.4.2 CODE */

#define GLAPIENTRY
#define GET_CURRENT_CONTEXT(x)
#define ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(x)
#define MESA_VERBOSE 0
#define VERBOSE_API 0
#define MAX_TEXTURE_UNITS 0
#define MAX_PROGRAM_MATRICES 0
#define _mesa_debug(...)
#define _mesa_error(...)
#define ASSERT_OUTSIDE_BEGIN_END(...)
#define FLUSH_VERTICES(...)
#define MEMCPY memcpy
#define _mesa_sin sin
#define _mesa_cos cos
#define SQRTF sqrt
#define FABSF fabs
#define CALLOC malloc
#define MAX_MODELVIEW_STACK_DEPTH 10
#define MAX_PROJECTION_STACK_DEPTH 10
#define MAX_COLOR_STACK_DEPTH 0
#define MAX_TEXTURE_STACK_DEPTH 0
#define MAX_PROGRAM_MATRIX_STACK_DEPTH 0
#define _math_matrix_alloc_inv(...)
#define ALIGN_MALLOC(x,y) malloc(x)
typedef double GLdouble; // MAYBE BETTER TO USE GLFLOAT?

/**
 * \name Bits to indicate what state has changed.  
 *
 * 4 unused flags.
 */
/*@{*/
#define _NEW_MODELVIEW		0x1        /**< __GLcontextRec::ModelView */
#define _NEW_PROJECTION		0x2        /**< __GLcontextRec::Projection */
#define _NEW_TEXTURE_MATRIX	0x4        /**< __GLcontextRec::TextureMatrix */
#define _NEW_COLOR_MATRIX	0x8        /**< __GLcontextRec::ColorMatrix */
#define _NEW_ACCUM		0x10       /**< __GLcontextRec::Accum */
#define _NEW_COLOR		0x20       /**< __GLcontextRec::Color */
#define _NEW_DEPTH		0x40       /**< __GLcontextRec::Depth */
#define _NEW_EVAL		0x80       /**< __GLcontextRec::Eval, __GLcontextRec::EvalMap */
#define _NEW_FOG		0x100      /**< __GLcontextRec::Fog */
#define _NEW_HINT		0x200      /**< __GLcontextRec::Hint */
#define _NEW_LIGHT		0x400      /**< __GLcontextRec::Light */
#define _NEW_LINE		0x800      /**< __GLcontextRec::Line */
#define _NEW_PIXEL		0x1000     /**< __GLcontextRec::Pixel */
#define _NEW_POINT		0x2000     /**< __GLcontextRec::Point */
#define _NEW_POLYGON		0x4000     /**< __GLcontextRec::Polygon */
#define _NEW_POLYGONSTIPPLE	0x8000     /**< __GLcontextRec::PolygonStipple */
#define _NEW_SCISSOR		0x10000    /**< __GLcontextRec::Scissor */
#define _NEW_STENCIL		0x20000    /**< __GLcontextRec::Stencil */
#define _NEW_TEXTURE		0x40000    /**< __GLcontextRec::Texture */
#define _NEW_TRANSFORM		0x80000    /**< __GLcontextRec::Transform */
#define _NEW_VIEWPORT		0x100000   /**< __GLcontextRec::Viewport */
#define _NEW_PACKUNPACK		0x200000   /**< __GLcontextRec::Pack, __GLcontextRec::Unpack */
#define _NEW_ARRAY	        0x400000   /**< __GLcontextRec::Array */
#define _NEW_RENDERMODE		0x800000   /**< __GLcontextRec::RenderMode, __GLcontextRec::Feedback, __GLcontextRec::Select */
#define _NEW_BUFFERS            0x1000000  /**< __GLcontextRec::Visual, __GLcontextRec::DrawBuffer, */
#define _NEW_MULTISAMPLE        0x2000000  /**< __GLcontextRec::Multisample */
#define _NEW_TRACK_MATRIX       0x4000000  /**< __GLcontextRec::VertexProgram */
#define _NEW_PROGRAM            0x8000000  /**< __GLcontextRec::VertexProgram */
#define _NEW_ALL ~0
/*@}*/

/**
 * Different kinds of 4x4 transformation matrices.
 * We use these to select specific optimized vertex transformation routines.
 */
enum GLmatrixtype {
    MATRIX_GENERAL,	/**< general 4x4 matrix */
    MATRIX_IDENTITY,	/**< identity matrix */
    MATRIX_3D_NO_ROT,	/**< orthogonal projection and others... */
    MATRIX_PERSPECTIVE,	/**< perspective projection matrix */
    MATRIX_2D,		/**< 2-D transformation */
    MATRIX_2D_NO_ROT,	/**< 2-D scale & translate only */
    MATRIX_3D		/**< 3-D transformation */
} ;

/**
 * Matrix type to represent 4x4 transformation matrices.
 */
typedef struct {
    GLfloat *m;		/**< 16 matrix elements (16-byte aligned) */
    GLfloat *inv;	/**< optional 16-element inverse (16-byte aligned) */
    GLuint flags;        /**< possible values determined by (of \link
                          * MatFlags MAT_FLAG_* flags\endlink)
                          */
    enum GLmatrixtype type;
} GLmatrix;

/**
 * A stack of matrices (projection, modelview, color, texture, etc).
 */
struct gl_matrix_stack
{
    GLmatrix *Top;      /**< points into Stack */
    GLmatrix *Stack;    /**< array [MaxDepth] of GLmatrix */
    GLuint Depth;       /**< 0 <= Depth < MaxDepth */
    GLuint MaxDepth;    /**< size of Stack[] array */
    GLuint DirtyFlag;   /**< _NEW_MODELVIEW or _NEW_PROJECTION, for example */
};

/**
 * Transformation attribute group (GL_TRANSFORM_BIT).
 */
// Modified for Jumpcore
struct gl_transform_attrib
{
    GLenum MatrixMode;				/**< Matrix mode */
    /* ... */
};

/**
 * Mesa rendering context.
 *
 * This is the central context data structure for Mesa.  Almost all
 * OpenGL state is contained in this structure.
 * Think of this as a base class from which device drivers will derive
 * sub classes.
 *
 * The GLcontext typedef names this structure.
 */
// HEAVILY EDITED FOR JUMPCORE
struct __GLcontextRec
{
    /* ... */
    
    /** \name The various 4x4 matrix stacks */
    /*@{*/
    struct gl_matrix_stack ModelviewMatrixStack; // In practice I think only these first two get used in Jumpcore?
    struct gl_matrix_stack ProjectionMatrixStack;
    struct gl_matrix_stack ColorMatrixStack;
    struct gl_matrix_stack TextureMatrixStack[MAX_TEXTURE_UNITS];
    struct gl_matrix_stack ProgramMatrixStack[MAX_PROGRAM_MATRICES];
    struct gl_matrix_stack *CurrentStack; /**< Points to one of the above stacks */
    /*@}*/
    
    /* ... */
    
    struct gl_transform_attrib	Transform;	/**< Transformation attributes */

    /* ... */
    
    GLbitfield NewState;      /**< bitwise-or of _NEW_* flags */
};

typedef __GLcontextRec GLcontext;

GLcontext _ctx;
GLcontext *ctx = &_ctx;

/** 
 * Test geometry related matrix flags.
 * 
 * \param mat a pointer to a GLmatrix structure.
 * \param a flags mask.
 *
 * \returns non-zero if all geometry related matrix flags are contained within
 * the mask, or zero otherwise.
 */ 
#define TEST_MAT_FLAGS(mat, a)  \
((MAT_FLAGS_GEOMETRY & (~(a)) & ((mat)->flags) ) == 0)

/**
 * \defgroup MatFlags MAT_FLAG_XXX-flags
 *
 * Bitmasks to indicate different kinds of 4x4 matrices in GLmatrix::flags
 * It would be nice to make all these flags private to m_matrix.c
 */
/*@{*/
#define MAT_FLAG_IDENTITY       0     /**< is an identity matrix flag.
*   (Not actually used - the identity
*   matrix is identified by the absense
*   of all other flags.)
*/
#define MAT_FLAG_GENERAL        0x1   /**< is a general matrix flag */
#define MAT_FLAG_ROTATION       0x2   /**< is a rotation matrix flag */
#define MAT_FLAG_TRANSLATION    0x4   /**< is a translation matrix flag */
#define MAT_FLAG_UNIFORM_SCALE  0x8   /**< is an uniform scaling matrix flag */
#define MAT_FLAG_GENERAL_SCALE  0x10  /**< is a general scaling matrix flag */
#define MAT_FLAG_GENERAL_3D     0x20  /**< general 3D matrix flag */
#define MAT_FLAG_PERSPECTIVE    0x40  /**< is a perspective proj matrix flag */
#define MAT_FLAG_SINGULAR       0x80  /**< is a singular matrix flag */
#define MAT_DIRTY_TYPE          0x100  /**< matrix type is dirty */
#define MAT_DIRTY_FLAGS         0x200  /**< matrix flags are dirty */
#define MAT_DIRTY_INVERSE       0x400  /**< matrix inverse is dirty */

/** angle preserving matrix flags mask */
#define MAT_FLAGS_ANGLE_PRESERVING (MAT_FLAG_ROTATION | \
MAT_FLAG_TRANSLATION | \
MAT_FLAG_UNIFORM_SCALE)

/** geometry related matrix flags mask */
#define MAT_FLAGS_GEOMETRY (MAT_FLAG_GENERAL | \
MAT_FLAG_ROTATION | \
MAT_FLAG_TRANSLATION | \
MAT_FLAG_UNIFORM_SCALE | \
MAT_FLAG_GENERAL_SCALE | \
MAT_FLAG_GENERAL_3D | \
MAT_FLAG_PERSPECTIVE | \
MAT_FLAG_SINGULAR)

/** length preserving matrix flags mask */
#define MAT_FLAGS_LENGTH_PRESERVING (MAT_FLAG_ROTATION | \
MAT_FLAG_TRANSLATION)


/** 3D (non-perspective) matrix flags mask */
#define MAT_FLAGS_3D (MAT_FLAG_ROTATION | \
MAT_FLAG_TRANSLATION | \
MAT_FLAG_UNIFORM_SCALE | \
MAT_FLAG_GENERAL_SCALE | \
MAT_FLAG_GENERAL_3D)

/** dirty matrix flags mask */
#define MAT_DIRTY          (MAT_DIRTY_TYPE | \
MAT_DIRTY_FLAGS | \
MAT_DIRTY_INVERSE)

/**
 * Identity matrix.
 */
static GLfloat Identity[16] = {
    1.0, 0.0, 0.0, 0.0,
    0.0, 1.0, 0.0, 0.0,
    0.0, 0.0, 1.0, 0.0,
    0.0, 0.0, 0.0, 1.0
};

/**********************************************************************/
/** \name Matrix multiplication */
/*@{*/

#define A(row,col)  a[(col<<2)+row]
#define B(row,col)  b[(col<<2)+row]
#define P(row,col)  product[(col<<2)+row]

/**
 * Perform a full 4x4 matrix multiplication.
 *
 * \param a matrix.
 * \param b matrix.
 * \param product will receive the product of \p a and \p b.
 *
 * \warning Is assumed that \p product != \p b. \p product == \p a is allowed.
 *
 * \note KW: 4*16 = 64 multiplications
 * 
 * \author This \c matmul was contributed by Thomas Malik
 */
static void matmul4( GLfloat *product, const GLfloat *a, const GLfloat *b )
{
    GLint i;
    for (i = 0; i < 4; i++) {
        const GLfloat ai0=A(i,0),  ai1=A(i,1),  ai2=A(i,2),  ai3=A(i,3);
        P(i,0) = ai0 * B(0,0) + ai1 * B(1,0) + ai2 * B(2,0) + ai3 * B(3,0);
        P(i,1) = ai0 * B(0,1) + ai1 * B(1,1) + ai2 * B(2,1) + ai3 * B(3,1);
        P(i,2) = ai0 * B(0,2) + ai1 * B(1,2) + ai2 * B(2,2) + ai3 * B(3,2);
        P(i,3) = ai0 * B(0,3) + ai1 * B(1,3) + ai2 * B(2,3) + ai3 * B(3,3);
    }
}

/**
 * Multiply two matrices known to occupy only the top three rows, such
 * as typical model matrices, and orthogonal matrices.
 *
 * \param a matrix.
 * \param b matrix.
 * \param product will receive the product of \p a and \p b.
 */
static void matmul34( GLfloat *product, const GLfloat *a, const GLfloat *b )
{
    GLint i;
    for (i = 0; i < 3; i++) {
        const GLfloat ai0=A(i,0),  ai1=A(i,1),  ai2=A(i,2),  ai3=A(i,3);
        P(i,0) = ai0 * B(0,0) + ai1 * B(1,0) + ai2 * B(2,0);
        P(i,1) = ai0 * B(0,1) + ai1 * B(1,1) + ai2 * B(2,1);
        P(i,2) = ai0 * B(0,2) + ai1 * B(1,2) + ai2 * B(2,2);
        P(i,3) = ai0 * B(0,3) + ai1 * B(1,3) + ai2 * B(2,3) + ai3;
    }
    P(3,0) = 0;
    P(3,1) = 0;
    P(3,2) = 0;
    P(3,3) = 1;
}

#undef A
#undef B
#undef P

/**
 * Multiply a matrix by an array of floats with known properties.
 *
 * \param mat pointer to a GLmatrix structure containing the left multiplication
 * matrix, and that will receive the product result.
 * \param m right multiplication matrix array.
 * \param flags flags of the matrix \p m.
 * 
 * Joins both flags and marks the type and inverse as dirty.  Calls matmul34()
 * if both matrices are 3D, or matmul4() otherwise.
 */
static void matrix_multf( GLmatrix *mat, const GLfloat *m, GLuint flags )
{
    mat->flags |= (flags | MAT_DIRTY_TYPE | MAT_DIRTY_INVERSE);
    
    if (TEST_MAT_FLAGS(mat, MAT_FLAGS_3D))
        matmul34( mat->m, mat->m, m );
    else
        matmul4( mat->m, mat->m, m );
}

/**
 * Set a matrix to the identity matrix.
 *
 * \param mat matrix.
 *
 * Copies ::Identity into \p GLmatrix::m, and into GLmatrix::inv if not NULL.
 * Sets the matrix type to identity, and clear the dirty flags.
 */
void
_math_matrix_set_identity( GLmatrix *mat )
{
    MEMCPY( mat->m, Identity, 16*sizeof(GLfloat) );
    
    if (mat->inv)
        MEMCPY( mat->inv, Identity, 16*sizeof(GLfloat) );
    
    mat->type = MATRIX_IDENTITY;
    mat->flags &= ~(MAT_DIRTY_FLAGS|
                    MAT_DIRTY_TYPE|
                    MAT_DIRTY_INVERSE);
}

#define DEG2RAD (M_PI/180.0)

/**
 * Copy a matrix.
 *
 * \param to destination matrix.
 * \param from source matrix.
 *
 * Copies all fields in GLmatrix, creating an inverse array if necessary.
 */
void
_math_matrix_copy( GLmatrix *to, const GLmatrix *from )
{
    MEMCPY( to->m, from->m, sizeof(Identity) );
    to->flags = from->flags;
    to->type = from->type;
    
    if (to->inv != 0) {
        if (from->inv == 0) {
#if 0
            matrix_invert( to );
#endif
        }
        else {
            MEMCPY(to->inv, from->inv, sizeof(GLfloat)*16);
        }
    }
}

/**********************************************************************/
/** \name Matrix generation */
/*@{*/

/**
 * Generate a 4x4 transformation matrix from glRotate parameters, and
 * post-multiply the input matrix by it.
 *
 * \author
 * This function was contributed by Erich Boleyn (erich@uruk.org).
 * Optimizations contributed by Rudolf Opalla (rudi@khm.de).
 */
void
_math_matrix_rotate( GLmatrix *mat,
                    GLfloat angle, GLfloat x, GLfloat y, GLfloat z )
{
    GLfloat xx, yy, zz, xy, yz, zx, xs, ys, zs, one_c, s, c;
    GLfloat m[16];
    GLboolean optimized;
    
    s = (GLfloat) _mesa_sin( angle * DEG2RAD );
    c = (GLfloat) _mesa_cos( angle * DEG2RAD );
    
    MEMCPY(m, Identity, sizeof(GLfloat)*16);
    optimized = GL_FALSE;
    
#define M(row,col)  m[col*4+row]
    
    if (x == 0.0F) {
        if (y == 0.0F) {
            if (z != 0.0F) {
                optimized = GL_TRUE;
                /* rotate only around z-axis */
                M(0,0) = c;
                M(1,1) = c;
                if (z < 0.0F) {
                    M(0,1) = s;
                    M(1,0) = -s;
                }
                else {
                    M(0,1) = -s;
                    M(1,0) = s;
                }
            }
        }
        else if (z == 0.0F) {
            optimized = GL_TRUE;
            /* rotate only around y-axis */
            M(0,0) = c;
            M(2,2) = c;
            if (y < 0.0F) {
                M(0,2) = -s;
                M(2,0) = s;
            }
            else {
                M(0,2) = s;
                M(2,0) = -s;
            }
        }
    }
    else if (y == 0.0F) {
        if (z == 0.0F) {
            optimized = GL_TRUE;
            /* rotate only around x-axis */
            M(1,1) = c;
            M(2,2) = c;
            if (x < 0.0F) {
                M(1,2) = s;
                M(2,1) = -s;
            }
            else {
                M(1,2) = -s;
                M(2,1) = s;
            }
        }
    }
    
    if (!optimized) {
        const GLfloat mag = SQRTF(x * x + y * y + z * z);
        
        if (mag <= 1.0e-4) {
            /* no rotation, leave mat as-is */
            return;
        }
        
        x /= mag;
        y /= mag;
        z /= mag;
        
        
        /*
         *     Arbitrary axis rotation matrix.
         *
         *  This is composed of 5 matrices, Rz, Ry, T, Ry', Rz', multiplied
         *  like so:  Rz * Ry * T * Ry' * Rz'.  T is the final rotation
         *  (which is about the X-axis), and the two composite transforms
         *  Ry' * Rz' and Rz * Ry are (respectively) the rotations necessary
         *  from the arbitrary axis to the X-axis then back.  They are
         *  all elementary rotations.
         *
         *  Rz' is a rotation about the Z-axis, to bring the axis vector
         *  into the x-z plane.  Then Ry' is applied, rotating about the
         *  Y-axis to bring the axis vector parallel with the X-axis.  The
         *  rotation about the X-axis is then performed.  Ry and Rz are
         *  simply the respective inverse transforms to bring the arbitrary
         *  axis back to it's original orientation.  The first transforms
         *  Rz' and Ry' are considered inverses, since the data from the
         *  arbitrary axis gives you info on how to get to it, not how
         *  to get away from it, and an inverse must be applied.
         *
         *  The basic calculation used is to recognize that the arbitrary
         *  axis vector (x, y, z), since it is of unit length, actually
         *  represents the sines and cosines of the angles to rotate the
         *  X-axis to the same orientation, with theta being the angle about
         *  Z and phi the angle about Y (in the order described above)
         *  as follows:
         *
         *  cos ( theta ) = x / sqrt ( 1 - z^2 )
         *  sin ( theta ) = y / sqrt ( 1 - z^2 )
         *
         *  cos ( phi ) = sqrt ( 1 - z^2 )
         *  sin ( phi ) = z
         *
         *  Note that cos ( phi ) can further be inserted to the above
         *  formulas:
         *
         *  cos ( theta ) = x / cos ( phi )
         *  sin ( theta ) = y / sin ( phi )
         *
         *  ...etc.  Because of those relations and the standard trigonometric
         *  relations, it is pssible to reduce the transforms down to what
         *  is used below.  It may be that any primary axis chosen will give the
         *  same results (modulo a sign convention) using thie method.
         *
         *  Particularly nice is to notice that all divisions that might
         *  have caused trouble when parallel to certain planes or
         *  axis go away with care paid to reducing the expressions.
         *  After checking, it does perform correctly under all cases, since
         *  in all the cases of division where the denominator would have
         *  been zero, the numerator would have been zero as well, giving
         *  the expected result.
         */
        
        xx = x * x;
        yy = y * y;
        zz = z * z;
        xy = x * y;
        yz = y * z;
        zx = z * x;
        xs = x * s;
        ys = y * s;
        zs = z * s;
        one_c = 1.0F - c;
        
        /* We already hold the identity-matrix so we can skip some statements */
        M(0,0) = (one_c * xx) + c;
        M(0,1) = (one_c * xy) - zs;
        M(0,2) = (one_c * zx) + ys;
        /*    M(0,3) = 0.0F; */
        
        M(1,0) = (one_c * xy) + zs;
        M(1,1) = (one_c * yy) + c;
        M(1,2) = (one_c * yz) - xs;
        /*    M(1,3) = 0.0F; */
        
        M(2,0) = (one_c * zx) - ys;
        M(2,1) = (one_c * yz) + xs;
        M(2,2) = (one_c * zz) + c;
        /*    M(2,3) = 0.0F; */
        
        /*
         M(3,0) = 0.0F;
         M(3,1) = 0.0F;
         M(3,2) = 0.0F;
         M(3,3) = 1.0F;
         */
    }
#undef M
    
    matrix_multf( mat, m, MAT_FLAG_ROTATION );
}

/**
 * Apply a perspective projection matrix.
 *
 * \param mat matrix to apply the projection.
 * \param left left clipping plane coordinate.
 * \param right right clipping plane coordinate.
 * \param bottom bottom clipping plane coordinate.
 * \param top top clipping plane coordinate.
 * \param nearval distance to the near clipping plane.
 * \param farval distance to the far clipping plane.
 *
 * Creates the projection matrix and multiplies it with \p mat, marking the
 * MAT_FLAG_PERSPECTIVE flag.
 */
void
_math_matrix_frustum( GLmatrix *mat,
                     GLfloat left, GLfloat right,
                     GLfloat bottom, GLfloat top,
                     GLfloat nearval, GLfloat farval )
{
    GLfloat x, y, a, b, c, d;
    GLfloat m[16];
    
    x = (2.0F*nearval) / (right-left);
    y = (2.0F*nearval) / (top-bottom);
    a = (right+left) / (right-left);
    b = (top+bottom) / (top-bottom);
    c = -(farval+nearval) / ( farval-nearval);
    d = -(2.0F*farval*nearval) / (farval-nearval);  /* error? */
    
#define M(row,col)  m[col*4+row]
    M(0,0) = x;     M(0,1) = 0.0F;  M(0,2) = a;      M(0,3) = 0.0F;
    M(1,0) = 0.0F;  M(1,1) = y;     M(1,2) = b;      M(1,3) = 0.0F;
    M(2,0) = 0.0F;  M(2,1) = 0.0F;  M(2,2) = c;      M(2,3) = d;
    M(3,0) = 0.0F;  M(3,1) = 0.0F;  M(3,2) = -1.0F;  M(3,3) = 0.0F;
#undef M
    
    matrix_multf( mat, m, MAT_FLAG_PERSPECTIVE );
}

/**
 * Apply an orthographic projection matrix.
 *
 * \param mat matrix to apply the projection.
 * \param left left clipping plane coordinate.
 * \param right right clipping plane coordinate.
 * \param bottom bottom clipping plane coordinate.
 * \param top top clipping plane coordinate.
 * \param nearval distance to the near clipping plane.
 * \param farval distance to the far clipping plane.
 *
 * Creates the projection matrix and multiplies it with \p mat, marking the
 * MAT_FLAG_GENERAL_SCALE and MAT_FLAG_TRANSLATION flags.
 */
void
_math_matrix_ortho( GLmatrix *mat,
                   GLfloat left, GLfloat right,
                   GLfloat bottom, GLfloat top,
                   GLfloat nearval, GLfloat farval )
{
    GLfloat m[16];
    
#define M(row,col)  m[col*4+row]
    M(0,0) = 2.0F / (right-left);
    M(0,1) = 0.0F;
    M(0,2) = 0.0F;
    M(0,3) = -(right+left) / (right-left);
    
    M(1,0) = 0.0F;
    M(1,1) = 2.0F / (top-bottom);
    M(1,2) = 0.0F;
    M(1,3) = -(top+bottom) / (top-bottom);
    
    M(2,0) = 0.0F;
    M(2,1) = 0.0F;
    M(2,2) = -2.0F / (farval-nearval);
    M(2,3) = -(farval+nearval) / (farval-nearval);
    
    M(3,0) = 0.0F;
    M(3,1) = 0.0F;
    M(3,2) = 0.0F;
    M(3,3) = 1.0F;
#undef M
    
    matrix_multf( mat, m, (MAT_FLAG_GENERAL_SCALE|MAT_FLAG_TRANSLATION));
}

/**
 * Multiply a matrix with a general scaling matrix.
 *
 * \param mat matrix.
 * \param x x axis scale factor.
 * \param y y axis scale factor.
 * \param z z axis scale factor.
 *
 * Multiplies in-place the elements of \p mat by the scale factors. Checks if
 * the scales factors are roughly the same, marking the MAT_FLAG_UNIFORM_SCALE
 * flag, or MAT_FLAG_GENERAL_SCALE. Marks the MAT_DIRTY_TYPE and
 * MAT_DIRTY_INVERSE dirty flags.
 */
void
_math_matrix_scale( GLmatrix *mat, GLfloat x, GLfloat y, GLfloat z )
{
    GLfloat *m = mat->m;
    m[0] *= x;   m[4] *= y;   m[8]  *= z;
    m[1] *= x;   m[5] *= y;   m[9]  *= z;
    m[2] *= x;   m[6] *= y;   m[10] *= z;
    m[3] *= x;   m[7] *= y;   m[11] *= z;
    
    if (FABSF(x - y) < 1e-8 && FABSF(x - z) < 1e-8)
        mat->flags |= MAT_FLAG_UNIFORM_SCALE;
    else
        mat->flags |= MAT_FLAG_GENERAL_SCALE;
    
    mat->flags |= (MAT_DIRTY_TYPE |
                   MAT_DIRTY_INVERSE);
}

/**
 * Multiply a matrix with a translation matrix.
 *
 * \param mat matrix.
 * \param x translation vector x coordinate.
 * \param y translation vector y coordinate.
 * \param z translation vector z coordinate.
 *
 * Adds the translation coordinates to the elements of \p mat in-place.  Marks
 * the MAT_FLAG_TRANSLATION flag, and the MAT_DIRTY_TYPE and MAT_DIRTY_INVERSE
 * dirty flags.
 */
void
_math_matrix_translate( GLmatrix *mat, GLfloat x, GLfloat y, GLfloat z )
{
    GLfloat *m = mat->m;
    m[12] = m[0] * x + m[4] * y + m[8]  * z + m[12];
    m[13] = m[1] * x + m[5] * y + m[9]  * z + m[13];
    m[14] = m[2] * x + m[6] * y + m[10] * z + m[14];
    m[15] = m[3] * x + m[7] * y + m[11] * z + m[15];
    
    mat->flags |= (MAT_FLAG_TRANSLATION |
                   MAT_DIRTY_TYPE |
                   MAT_DIRTY_INVERSE);
}

/**
 * Apply a perspective projection matrix.
 *
 * \param left left clipping plane coordinate.
 * \param right right clipping plane coordinate.
 * \param bottom bottom clipping plane coordinate.
 * \param top top clipping plane coordinate.
 * \param nearval distance to the near clipping plane.
 * \param farval distance to the far clipping plane.
 *
 * \sa glFrustum().
 *
 * Flushes vertices and validates parameters. Calls _math_matrix_frustum() with
 * the top matrix of the current matrix stack and sets
 * __GLcontextRec::NewState.
 */
void GLAPIENTRY
_mesa_Frustum( GLdouble left, GLdouble right,
              GLdouble bottom, GLdouble top,
              GLdouble nearval, GLdouble farval )
{
    GET_CURRENT_CONTEXT(ctx);
    ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);
    
    if (nearval <= 0.0 ||
        farval <= 0.0 ||
        nearval == farval ||
        left == right ||
        top == bottom)
    {
        _mesa_error( ctx,  GL_INVALID_VALUE, "glFrustum" );
        return;
    }
    
    _math_matrix_frustum( ctx->CurrentStack->Top,
                         (GLfloat) left, (GLfloat) right, 
                         (GLfloat) bottom, (GLfloat) top, 
                         (GLfloat) nearval, (GLfloat) farval );
    ctx->NewState |= ctx->CurrentStack->DirtyFlag;
}


/**
 * Apply an orthographic projection matrix.
 *
 * \param left left clipping plane coordinate.
 * \param right right clipping plane coordinate.
 * \param bottom bottom clipping plane coordinate.
 * \param top top clipping plane coordinate.
 * \param nearval distance to the near clipping plane.
 * \param farval distance to the far clipping plane.
 *
 * \sa glOrtho().
 *
 * Flushes vertices and validates parameters. Calls _math_matrix_ortho() with
 * the top matrix of the current matrix stack and sets
 * __GLcontextRec::NewState.
 */
void GLAPIENTRY
_mesa_Ortho( GLdouble left, GLdouble right,
            GLdouble bottom, GLdouble top,
            GLdouble nearval, GLdouble farval )
{
    GET_CURRENT_CONTEXT(ctx);
    ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);
    
    if (MESA_VERBOSE & VERBOSE_API)
        _mesa_debug(ctx, "glOrtho(%f, %f, %f, %f, %f, %f)\n",
                    left, right, bottom, top, nearval, farval);
    
    if (left == right ||
        bottom == top ||
        nearval == farval)
    {
        _mesa_error( ctx,  GL_INVALID_VALUE, "glOrtho" );
        return;
    }
    
    _math_matrix_ortho( ctx->CurrentStack->Top,
                       (GLfloat) left, (GLfloat) right, 
                       (GLfloat) bottom, (GLfloat) top, 
                       (GLfloat) nearval, (GLfloat) farval );
    ctx->NewState |= ctx->CurrentStack->DirtyFlag;
}

// From glu/project.c
void GLAPIENTRY
_mesa_Perspective(GLdouble fovy, GLdouble aspect, GLdouble zNear, GLdouble zFar)
{
    GLfloat m[4][4];
    GLfloat sine, cotangent, deltaZ;
    GLfloat radians = fovy / 2 * M_PI / 180;

    deltaZ = zFar - zNear;
    sine = sin(radians);
    if ((deltaZ == 0) || (sine == 0) || (aspect == 0)) {
	return;
    }
    cotangent = cos(radians) / sine;

	MEMCPY( &m[0][0], Identity, 16*sizeof(GLfloat) );
    m[0][0] = cotangent / aspect;
    m[1][1] = cotangent;
    m[2][2] = -(zFar + zNear) / deltaZ;
    m[2][3] = -1;
    m[3][2] = -2 * zNear * zFar / deltaZ;
    m[3][3] = 0;
	
	matrix_multf( ctx->CurrentStack->Top, &m[0][0], (MAT_FLAG_PERSPECTIVE|MAT_FLAG_GENERAL_SCALE|MAT_FLAG_TRANSLATION) );
	ctx->CurrentStack->Top->type = MATRIX_PERSPECTIVE;
	ctx->NewState |= ctx->CurrentStack->DirtyFlag;
}

/**
 * Set the current matrix stack.
 *
 * \param mode matrix stack.
 *
 * \sa glMatrixMode().
 *
 * Flushes the vertices, validates the parameter and updates
 * __GLcontextRec::CurrentStack and gl_transform_attrib::MatrixMode with the
 * specified matrix stack.
 */
// Modified for Jumpcore
void GLAPIENTRY
_mesa_MatrixMode( GLenum mode )
{
    GET_CURRENT_CONTEXT(ctx);
    ASSERT_OUTSIDE_BEGIN_END(ctx);
    
    if (ctx->Transform.MatrixMode == mode && mode != GL_TEXTURE)
        return;
    FLUSH_VERTICES(ctx, _NEW_TRANSFORM);
    
    switch (mode) {
        case GL_MODELVIEW:
            ctx->CurrentStack = &ctx->ModelviewMatrixStack;
            break;
        case GL_PROJECTION:
            ctx->CurrentStack = &ctx->ProjectionMatrixStack;
            break;
#if 0
        case GL_TEXTURE:
            if (ctx->Texture.CurrentUnit >= ctx->Const.MaxTextureCoordUnits) {
                _mesa_error(ctx, GL_INVALID_OPERATION, "glMatrixMode(texcoord unit)");
                return;
            }
            ctx->CurrentStack = &ctx->TextureMatrixStack[ctx->Texture.CurrentUnit];
            break;
        case GL_COLOR:
            ctx->CurrentStack = &ctx->ColorMatrixStack;
            break;
        case GL_MATRIX0_NV:
        case GL_MATRIX1_NV:
        case GL_MATRIX2_NV:
        case GL_MATRIX3_NV:
        case GL_MATRIX4_NV:
        case GL_MATRIX5_NV:
        case GL_MATRIX6_NV:
        case GL_MATRIX7_NV:
            if (ctx->Extensions.NV_vertex_program) {
                ctx->CurrentStack = &ctx->ProgramMatrixStack[mode - GL_MATRIX0_NV];
            }
            else {
                _mesa_error( ctx,  GL_INVALID_ENUM, "glMatrixMode(mode)" );
                return;
            }
            break;
        case GL_MATRIX0_ARB:
        case GL_MATRIX1_ARB:
        case GL_MATRIX2_ARB:
        case GL_MATRIX3_ARB:
        case GL_MATRIX4_ARB:
        case GL_MATRIX5_ARB:
        case GL_MATRIX6_ARB:
        case GL_MATRIX7_ARB:
            if (ctx->Extensions.ARB_vertex_program ||
                ctx->Extensions.ARB_fragment_program) {
                const GLuint m = mode - GL_MATRIX0_ARB;
                if (m > ctx->Const.MaxProgramMatrices) {
                    _mesa_error(ctx, GL_INVALID_ENUM,
                                "glMatrixMode(GL_MATRIX%d_ARB)", m);
                    return;
                }
                ctx->CurrentStack = &ctx->ProgramMatrixStack[m];
            }
            else {
                _mesa_error( ctx,  GL_INVALID_ENUM, "glMatrixMode(mode)" );
                return;
            }
            break;
#endif
        default:
            _mesa_error( ctx,  GL_INVALID_ENUM, "glMatrixMode(mode)" );
            return;
    }
    
    ctx->Transform.MatrixMode = mode;
}


/**
 * Push the current matrix stack.
 *
 * \sa glPushMatrix().
 * 
 * Verifies the current matrix stack is not full, and duplicates the top-most
 * matrix in the stack. Marks __GLcontextRec::NewState with the stack dirty
 * flag.
 */
void GLAPIENTRY
_mesa_PushMatrix( void )
{
    GET_CURRENT_CONTEXT(ctx);
    struct gl_matrix_stack *stack = ctx->CurrentStack;
    ASSERT_OUTSIDE_BEGIN_END(ctx);
    
    if (MESA_VERBOSE&VERBOSE_API)
        _mesa_debug(ctx, "glPushMatrix %s\n",
                    _mesa_lookup_enum_by_nr(ctx->Transform.MatrixMode));
    
    if (stack->Depth + 1 >= stack->MaxDepth) {
        if (ctx->Transform.MatrixMode == GL_TEXTURE) {
            _mesa_error(ctx,  GL_STACK_OVERFLOW,
                        "glPushMatrix(mode=GL_TEXTURE, unit=%d)",
                        ctx->Texture.CurrentUnit);
        }
        else {
            _mesa_error(ctx,  GL_STACK_OVERFLOW, "glPushMatrix(mode=%s)",
                        _mesa_lookup_enum_by_nr(ctx->Transform.MatrixMode));
        }
        return;
    }
    _math_matrix_copy( &stack->Stack[stack->Depth + 1],
                      &stack->Stack[stack->Depth] );
    stack->Depth++;
    stack->Top = &(stack->Stack[stack->Depth]);
    ctx->NewState |= stack->DirtyFlag;
}


/**
 * Pop the current matrix stack.
 *
 * \sa glPopMatrix().
 * 
 * Flushes the vertices, verifies the current matrix stack is not empty, and
 * moves the stack head down. Marks __GLcontextRec::NewState with the dirty
 * stack flag.
 */
void GLAPIENTRY
_mesa_PopMatrix( void )
{
    GET_CURRENT_CONTEXT(ctx);
    struct gl_matrix_stack *stack = ctx->CurrentStack;
    ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);
    
    if (MESA_VERBOSE&VERBOSE_API)
        _mesa_debug(ctx, "glPopMatrix %s\n",
                    _mesa_lookup_enum_by_nr(ctx->Transform.MatrixMode));
    
    if (stack->Depth == 0) {
        if (ctx->Transform.MatrixMode == GL_TEXTURE) {
            _mesa_error(ctx,  GL_STACK_UNDERFLOW,
                        "glPopMatrix(mode=GL_TEXTURE, unit=%d)",
                        ctx->Texture.CurrentUnit);
        }
        else {
            _mesa_error(ctx,  GL_STACK_UNDERFLOW, "glPopMatrix(mode=%s)",
                        _mesa_lookup_enum_by_nr(ctx->Transform.MatrixMode));
        }
        return;
    }
    stack->Depth--;
    stack->Top = &(stack->Stack[stack->Depth]);
    ctx->NewState |= stack->DirtyFlag;
}

/**
 * Replace the current matrix with the identity matrix.
 *
 * \sa glLoadIdentity().
 *
 * Flushes the vertices and calls _math_matrix_set_identity() with the top-most
 * matrix in the current stack. Marks __GLcontextRec::NewState with the stack
 * dirty flag.
 */
void GLAPIENTRY
_mesa_LoadIdentity( void )
{
    GET_CURRENT_CONTEXT(ctx);
    ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);
    
    if (MESA_VERBOSE & VERBOSE_API)
        _mesa_debug(ctx, "glLoadIdentity()");
    
    _math_matrix_set_identity( ctx->CurrentStack->Top );
    ctx->NewState |= ctx->CurrentStack->DirtyFlag;
}

/**
 * Multiply the current matrix with a rotation matrix.
 *
 * \param angle angle of rotation, in degrees.
 * \param x rotation vector x coordinate.
 * \param y rotation vector y coordinate.
 * \param z rotation vector z coordinate.
 *
 * \sa glRotatef().
 *
 * Flushes the vertices and calls _math_matrix_rotate() with the top-most
 * matrix in the current stack and the given parameters. Marks
 * __GLcontextRec::NewState with the dirty stack flag.
 */
void GLAPIENTRY
_mesa_Rotatef( GLfloat angle, GLfloat x, GLfloat y, GLfloat z )
{
    GET_CURRENT_CONTEXT(ctx);
    ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);
    if (angle != 0.0F) {
        _math_matrix_rotate( ctx->CurrentStack->Top, angle, x, y, z);
        ctx->NewState |= ctx->CurrentStack->DirtyFlag;
    }
}


/**
 * Multiply the current matrix with a general scaling matrix.
 *
 * \param x x axis scale factor.
 * \param y y axis scale factor.
 * \param z z axis scale factor.
 *
 * \sa glScalef().
 *
 * Flushes the vertices and calls _math_matrix_scale() with the top-most
 * matrix in the current stack and the given parameters. Marks
 * __GLcontextRec::NewState with the dirty stack flag.
 */
void GLAPIENTRY
_mesa_Scalef( GLfloat x, GLfloat y, GLfloat z )
{
    GET_CURRENT_CONTEXT(ctx);
    ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);
    _math_matrix_scale( ctx->CurrentStack->Top, x, y, z);
    ctx->NewState |= ctx->CurrentStack->DirtyFlag;
}


/**
 * Multiply the current matrix with a translation matrix.
 *
 * \param x translation vector x coordinate.
 * \param y translation vector y coordinate.
 * \param z translation vector z coordinate.
 *
 * \sa glTranslatef().
 *
 * Flushes the vertices and calls _math_matrix_translate() with the top-most
 * matrix in the current stack and the given parameters. Marks
 * __GLcontextRec::NewState with the dirty stack flag.
 */
void GLAPIENTRY
_mesa_Translatef( GLfloat x, GLfloat y, GLfloat z )
{
    GET_CURRENT_CONTEXT(ctx);
    ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);
    _math_matrix_translate( ctx->CurrentStack->Top, x, y, z);
    ctx->NewState |= ctx->CurrentStack->DirtyFlag;
}

/**
 * Matrix constructor.
 *
 * \param m matrix.
 *
 * Initialize the GLmatrix fields.
 */
void
_math_matrix_ctr( GLmatrix *m )
{
    m->m = (GLfloat *) ALIGN_MALLOC( 16 * sizeof(GLfloat), 16 );
    if (m->m)
        MEMCPY( m->m, Identity, sizeof(Identity) );
    m->inv = NULL;
    m->type = MATRIX_IDENTITY;
    m->flags = 0;
}

/**
 * Initialize a matrix stack.
 *
 * \param stack matrix stack.
 * \param maxDepth maximum stack depth.
 * \param dirtyFlag dirty flag.
 * 
 * Allocates an array of \p maxDepth elements for the matrix stack and calls
 * _math_matrix_ctr() and _math_matrix_alloc_inv() for each element to
 * initialize it.
 */
static void
init_matrix_stack( struct gl_matrix_stack *stack,
                  GLuint maxDepth, GLuint dirtyFlag )
{
    GLuint i;
    
    stack->Depth = 0;
    stack->MaxDepth = maxDepth;
    stack->DirtyFlag = dirtyFlag;
    /* The stack */
    stack->Stack = (GLmatrix *) CALLOC(maxDepth * sizeof(GLmatrix));
    for (i = 0; i < maxDepth; i++) {
        _math_matrix_ctr(&stack->Stack[i]);
        _math_matrix_alloc_inv(&stack->Stack[i]);
    }
    stack->Top = stack->Stack;
}

/**
 * Initialize the context matrix data.
 *
 * \param ctx GL context.
 *
 * Initializes each of the matrix stacks and the combined modelview-projection
 * matrix.
 */
void _mesa_init_matrix( GLcontext * ctx )
{
    GLint i;
    
    /* Initialize matrix stacks */
    init_matrix_stack(&ctx->ModelviewMatrixStack, MAX_MODELVIEW_STACK_DEPTH,
                      _NEW_MODELVIEW);
    init_matrix_stack(&ctx->ProjectionMatrixStack, MAX_PROJECTION_STACK_DEPTH,
                      _NEW_PROJECTION);
    init_matrix_stack(&ctx->ColorMatrixStack, MAX_COLOR_STACK_DEPTH,
                      _NEW_COLOR_MATRIX);
    for (i = 0; i < MAX_TEXTURE_UNITS; i++)
        init_matrix_stack(&ctx->TextureMatrixStack[i], MAX_TEXTURE_STACK_DEPTH,
                          _NEW_TEXTURE_MATRIX);
    for (i = 0; i < MAX_PROGRAM_MATRICES; i++)
        init_matrix_stack(&ctx->ProgramMatrixStack[i], 
                          MAX_PROGRAM_MATRIX_STACK_DEPTH, _NEW_TRACK_MATRIX);
    ctx->CurrentStack = &ctx->ModelviewMatrixStack;
    
    /* Init combined Modelview*Projection matrix */
    // Jumpcore doesn't use this?
//    _math_matrix_ctr( &ctx->_ModelProjectMatrix );
}

/** 
 * Initialize the context transform attribute group.
 *
 * \param ctx GL context.
 *
 * \todo Move this to a new file with other 'transform' routines.
 */
// Modified for Jumpcore
void _mesa_init_transform( GLcontext *ctx )
{
    /* ... */
    /* Transformation group */
    ctx->Transform.MatrixMode = GL_MODELVIEW;
    /* ... */
}

void mesa_setup() {
    ctx->NewState = _NEW_ALL;
    _mesa_init_transform( ctx );
    _mesa_init_matrix( ctx );
}

void mesa_sync() {
    // TODO: Only when dirty
    GLfloat outmat[16];
    matmul4(outmat, ctx->ProjectionMatrixStack.Top->m, ctx->ModelviewMatrixStack.Top->m);
    glUniformMatrix4fv( p->uniforms[s_mvp_matrix], 1, GL_FALSE, outmat );
}

/* END MESA */
// Wow the remaining parts of this file are really short!!

void jcMatrixInit() {
    mesa_setup();
}

void jcMatrixMode(GLenum m) {
    if (gl2) {
        _mesa_MatrixMode(m);
    } else {
#ifndef FORCE_ES2
        glMatrixMode(m);
#endif
    }
}
void jcLoadIdentity() {
    if (gl2) {
        _mesa_LoadIdentity();
    } else {
#ifndef FORCE_ES2
        glLoadIdentity();
#endif
    }
}
void jcScalef(GLfloat x, GLfloat y, GLfloat z) {
    if (gl2) {
        _mesa_Scalef(x,y,z);
    } else {
#ifndef FORCE_ES2
        glScalef(x,y,z);
#endif
    }
}
void jcFrustumf(GLfloat xmin, GLfloat xmax, GLfloat ymin, GLfloat ymax, GLfloat zNear, GLfloat zFar) {
    if (gl2) {
        _mesa_Frustum(xmin,xmax,ymin,ymax,zNear,zFar);
    } else {
#ifndef FORCE_ES2
#ifdef TARGET_DESKTOP
        glFrustum(xmin,xmax,ymin,ymax,zNear,zFar);
#else
        glFrustumf(xmin,xmax,ymin,ymax,zNear,zFar);
#endif
#endif
    }
}
void jcOrtho(GLfloat a, GLfloat b, GLfloat c, GLfloat d, GLfloat e, GLfloat f) {
    if (gl2) {
        _mesa_Ortho(a,b,c,d,e,f);
    } else {
#ifndef FORCE_ES2
#ifdef TARGET_DESKTOP
        glOrtho(a,b,c,d,e,f);
#else
        glOrthof(a,b,c,d,e,f);
#endif
#endif
    }
}
void jcPerspective(GLfloat a, GLfloat b, GLfloat c, GLfloat d) {
    if (gl2) {
        _mesa_Perspective(a,b,c,d);
    } else {
#ifndef FORCE_ES2
		gluPerspective(a,b,c,d);
#endif
    }
}
void jcRotatef(GLfloat a,GLfloat b,GLfloat c,GLfloat d) {
    if (gl2) {
        _mesa_Rotatef(a,b,c,d);
    } else {
#ifndef FORCE_ES2
        glRotatef(a,b,c,d);
#endif
    }
}
void jcTranslatef(GLfloat a,GLfloat b,GLfloat c) {
    if (gl2) {
        _mesa_Translatef(a, b, c);
    } else {
#ifndef FORCE_ES2
        glTranslatef(a,b,c);
#endif
    }
}
void jcMultMatrixf(const GLfloat *f) {
	if (gl2) {
		matrix_multf( ctx->CurrentStack->Top, f, (MAT_FLAG_PERSPECTIVE|MAT_FLAG_GENERAL_SCALE|MAT_FLAG_TRANSLATION) );
		ctx->CurrentStack->Top->type = MATRIX_PERSPECTIVE;
		ctx->NewState |= ctx->CurrentStack->DirtyFlag;
    } else {
#ifndef FORCE_ES2
        glMultMatrixf(f);
#endif
    }
}
void jcPopMatrix() {
    if (gl2) {
        _mesa_PopMatrix();
    } else {
#ifndef FORCE_ES2
        glPopMatrix();
#endif
    }
}
void jcPushMatrix() {
    if (gl2) {
        _mesa_PushMatrix();
    } else {
#ifndef FORCE_ES2
        glPushMatrix();
#endif
    }
}

void jc1MatrixDump(GLfloat *mat, const char *name, const int matside) {
    ERR("%s:\n", name);
    for(int c = 0; c < matside*matside; c++) {
        ERR("\t%f", mat[c]);
        if (0==(c+1)%matside)
            ERR("\n");
        else
            ERR(",");            
    }
}

void jcMatrixFetch(GLfloat *projection, GLfloat *modelview, GLint *viewport) {
    if (gl2) {
#if 1
        const int matside = 4;
        const int matlen = matside*matside;
        
        if (projection)
            memcpy(projection, ctx->ProjectionMatrixStack.Top->m, matlen*sizeof(float));
        if (modelview)
            memcpy(modelview, ctx->ModelviewMatrixStack.Top->m, matlen*sizeof(float));
#else
        if (projection)
            glGetUniformfv( p_basic, s_projection_matrix, projection );
        if (modelview)
            glGetUniformfv( p_basic, s_modelview_matrix, modelview );        
#endif
    } else {
        if (projection)
            glGetFloatv(GL_PROJECTION_MATRIX, projection);
        if (modelview)
            glGetFloatv(GL_MODELVIEW_MATRIX, modelview);
    }
    if (viewport)
        glGetIntegerv(GL_VIEWPORT, viewport);        
}

// For debugging
void jcMatrixDump(const char *why) {
    const int matside = 4;
    const int matlen = matside*matside;
    GLfloat projection[matlen];
    GLfloat modelview[matlen];
    
    jcMatrixFetch(projection, modelview, NULL);

    ERR("Matrices %s%s\n", why?"@":"", why);
    jc1MatrixDump(projection, "PROJ", matside);
    jc1MatrixDump(modelview, "MODV", matside);
    ERR("\n");
    return; // Gives me something to hang a breakpoint on
}

void jcMatrixGlmIn(const glm::mat4 &v) {
	jcMatrixMode(GL_PROJECTION);
	jcLoadIdentity();
	jcMatrixMode(GL_MODELVIEW);
	jcLoadIdentity();
	jcMultMatrixf(glm::value_ptr(v));
}

#include "chipmunk.h"

#ifdef TARGET_WEBOS
static void __gluMultMatrixVecd(const GLdouble matrix[16], const GLdouble in[4],
		      GLdouble out[4])
{
    int i;

    for (i=0; i<4; i++) {
	out[i] = 
	    in[0] * matrix[0*4+i] +
	    in[1] * matrix[1*4+i] +
	    in[2] * matrix[2*4+i] +
	    in[3] * matrix[3*4+i];
    }
}

/*
** Invert 4x4 matrix.
** Contributed by David Moore (See Mesa bug #6748)
*/
static int __gluInvertMatrixd(const GLdouble m[16], GLdouble invOut[16])
{
    double inv[16], det;
    int i;

    inv[0] =   m[5]*m[10]*m[15] - m[5]*m[11]*m[14] - m[9]*m[6]*m[15]
             + m[9]*m[7]*m[14] + m[13]*m[6]*m[11] - m[13]*m[7]*m[10];
    inv[4] =  -m[4]*m[10]*m[15] + m[4]*m[11]*m[14] + m[8]*m[6]*m[15]
             - m[8]*m[7]*m[14] - m[12]*m[6]*m[11] + m[12]*m[7]*m[10];
    inv[8] =   m[4]*m[9]*m[15] - m[4]*m[11]*m[13] - m[8]*m[5]*m[15]
             + m[8]*m[7]*m[13] + m[12]*m[5]*m[11] - m[12]*m[7]*m[9];
    inv[12] = -m[4]*m[9]*m[14] + m[4]*m[10]*m[13] + m[8]*m[5]*m[14]
             - m[8]*m[6]*m[13] - m[12]*m[5]*m[10] + m[12]*m[6]*m[9];
    inv[1] =  -m[1]*m[10]*m[15] + m[1]*m[11]*m[14] + m[9]*m[2]*m[15]
             - m[9]*m[3]*m[14] - m[13]*m[2]*m[11] + m[13]*m[3]*m[10];
    inv[5] =   m[0]*m[10]*m[15] - m[0]*m[11]*m[14] - m[8]*m[2]*m[15]
             + m[8]*m[3]*m[14] + m[12]*m[2]*m[11] - m[12]*m[3]*m[10];
    inv[9] =  -m[0]*m[9]*m[15] + m[0]*m[11]*m[13] + m[8]*m[1]*m[15]
             - m[8]*m[3]*m[13] - m[12]*m[1]*m[11] + m[12]*m[3]*m[9];
    inv[13] =  m[0]*m[9]*m[14] - m[0]*m[10]*m[13] - m[8]*m[1]*m[14]
             + m[8]*m[2]*m[13] + m[12]*m[1]*m[10] - m[12]*m[2]*m[9];
    inv[2] =   m[1]*m[6]*m[15] - m[1]*m[7]*m[14] - m[5]*m[2]*m[15]
             + m[5]*m[3]*m[14] + m[13]*m[2]*m[7] - m[13]*m[3]*m[6];
    inv[6] =  -m[0]*m[6]*m[15] + m[0]*m[7]*m[14] + m[4]*m[2]*m[15]
             - m[4]*m[3]*m[14] - m[12]*m[2]*m[7] + m[12]*m[3]*m[6];
    inv[10] =  m[0]*m[5]*m[15] - m[0]*m[7]*m[13] - m[4]*m[1]*m[15]
             + m[4]*m[3]*m[13] + m[12]*m[1]*m[7] - m[12]*m[3]*m[5];
    inv[14] = -m[0]*m[5]*m[14] + m[0]*m[6]*m[13] + m[4]*m[1]*m[14]
             - m[4]*m[2]*m[13] - m[12]*m[1]*m[6] + m[12]*m[2]*m[5];
    inv[3] =  -m[1]*m[6]*m[11] + m[1]*m[7]*m[10] + m[5]*m[2]*m[11]
             - m[5]*m[3]*m[10] - m[9]*m[2]*m[7] + m[9]*m[3]*m[6];
    inv[7] =   m[0]*m[6]*m[11] - m[0]*m[7]*m[10] - m[4]*m[2]*m[11]
             + m[4]*m[3]*m[10] + m[8]*m[2]*m[7] - m[8]*m[3]*m[6];
    inv[11] = -m[0]*m[5]*m[11] + m[0]*m[7]*m[9] + m[4]*m[1]*m[11]
             - m[4]*m[3]*m[9] - m[8]*m[1]*m[7] + m[8]*m[3]*m[5];
    inv[15] =  m[0]*m[5]*m[10] - m[0]*m[6]*m[9] - m[4]*m[1]*m[10]
             + m[4]*m[2]*m[9] + m[8]*m[1]*m[6] - m[8]*m[2]*m[5];

    det = m[0]*inv[0] + m[1]*inv[4] + m[2]*inv[8] + m[3]*inv[12];
    if (det == 0)
        return GL_FALSE;

    det = 1.0 / det;

    for (i = 0; i < 16; i++)
        invOut[i] = inv[i] * det;

    return GL_TRUE;
}

static void __gluMultMatricesd(const GLdouble a[16], const GLdouble b[16],
				GLdouble r[16])
{
    int i, j;

    for (i = 0; i < 4; i++) {
	for (j = 0; j < 4; j++) {
	    r[i*4+j] = 
		a[i*4+0]*b[0*4+j] +
		a[i*4+1]*b[1*4+j] +
		a[i*4+2]*b[2*4+j] +
		a[i*4+3]*b[3*4+j];
	}
    }
}

GLint GLAPIENTRY
gluProject(GLdouble objx, GLdouble objy, GLdouble objz, 
	      const GLdouble modelMatrix[16], 
	      const GLdouble projMatrix[16],
              const GLint viewport[4],
	      GLdouble *winx, GLdouble *winy, GLdouble *winz)
{
    double in[4];
    double out[4];

    in[0]=objx;
    in[1]=objy;
    in[2]=objz;
    in[3]=1.0;
    __gluMultMatrixVecd(modelMatrix, in, out);
    __gluMultMatrixVecd(projMatrix, out, in);
    if (in[3] == 0.0) return(GL_FALSE);
    in[0] /= in[3];
    in[1] /= in[3];
    in[2] /= in[3];
    /* Map x, y and z to range 0-1 */
    in[0] = in[0] * 0.5 + 0.5;
    in[1] = in[1] * 0.5 + 0.5;
    in[2] = in[2] * 0.5 + 0.5;

    /* Map x,y to viewport */
    in[0] = in[0] * viewport[2] + viewport[0];
    in[1] = in[1] * viewport[3] + viewport[1];

    *winx=in[0];
    *winy=in[1];
    *winz=in[2];
    return(GL_TRUE);
}

GLint GLAPIENTRY
gluUnProject(GLdouble winx, GLdouble winy, GLdouble winz,
		const GLdouble modelMatrix[16], 
		const GLdouble projMatrix[16],
                const GLint viewport[4],
	        GLdouble *objx, GLdouble *objy, GLdouble *objz)
{
    double finalMatrix[16];
    double in[4];
    double out[4];

    __gluMultMatricesd(modelMatrix, projMatrix, finalMatrix);
    if (!__gluInvertMatrixd(finalMatrix, finalMatrix)) return(GL_FALSE);

    in[0]=winx;
    in[1]=winy;
    in[2]=winz;
    in[3]=1.0;

    /* Map x and y from window coordinates */
    in[0] = (in[0] - viewport[0]) / viewport[2];
    in[1] = (in[1] - viewport[1]) / viewport[3];

    /* Map to range -1 to 1 */
    in[0] = in[0] * 2 - 1;
    in[1] = in[1] * 2 - 1;
    in[2] = in[2] * 2 - 1;

    __gluMultMatrixVecd(finalMatrix, in, out);
    if (out[3] == 0.0) return(GL_FALSE);
    out[0] /= out[3];
    out[1] /= out[3];
    out[2] /= out[3];
    *objx = out[0];
    *objy = out[1];
    *objz = out[2];
    return(GL_TRUE);
}

#endif

// Converts an x,y point (such as a mouse click) with the current projection+modelview matrix.
cpVect screenToGL(int screenx, int screeny, double desiredZ) {
#if TARGET_ANDROID
	// Oh, the horrors of the Android emulator...
	// So the Android emulator supports OpenGL ES 1.0 only (as opposed to 1.1), which means it does not
	// support a particular function critical for this function to work. To get around this a brief,
	// incorrect screen-to-GL transformation is done here that will work ONLY with the coordinate space
	// of the interface library. This exists only so that I can test jumpcore.
	// TODO: This should actually check to see if we're in the emulator or not; otherwise, shipping code should IF 0 this bit
	if (!gl2) {
		extern int surfacew, surfaceh;
		extern double aspect;
		cpVect v = cpv(cpFloat(screenx)/surfacew, cpFloat(screeny)/surfaceh);
		v = cpvadd( cpvmult(v, 2), cpv(-1,-1) ); 
		v.x /= aspect; v.y = -v.y;
//		ERR("CLICK %lf, %lf", (double)v.x, (double)v.y);
		return v;
	}
#endif
	GLdouble model_view[16];
	GLdouble projection[16];
	GLint viewport[4];	    
	if (gl2) {
		GLfloat model_view_2[16];
		GLfloat projection_2[16];
		jcMatrixFetch(model_view_2, projection_2, viewport);
		for(int c = 0; c < 16; c++) { // THIS IS STUPID
			model_view[c] = model_view_2[c];
			projection[c] = projection_2[c];
		}		
	} else {
#ifndef TARGET_MOBILE
		glGetDoublev(GL_MODELVIEW_MATRIX, model_view);
		glGetDoublev(GL_PROJECTION_MATRIX, projection);
#else
		GLfloat model_view_2[16];
		GLfloat projection_2[16];
		glGetFloatv(GL_MODELVIEW_MATRIX, model_view_2);
		glGetFloatv(GL_PROJECTION_MATRIX, projection_2);
		for(int c = 0; c < 16; c++) { // THIS IS STUPID
			model_view[c] = model_view_2[c];
			projection[c] = projection_2[c];
		}
#endif
	}
    glGetIntegerv(GL_VIEWPORT, viewport);
	GLdouble x, y, buffZ, tempX, tempY, tempZ;
	
	gluProject(0,0, desiredZ, model_view, projection, viewport, &tempX, &tempY, &buffZ);
	gluUnProject(screenx, screeny, buffZ, model_view, projection, viewport, &x, &y, &tempZ);
	
    //	ERR("screenToGL x %d y %d -> x %lf, y %lf\n", (int)screenx, (int) screeny, x, y);
	
	return cpv(x, -y);	
}
