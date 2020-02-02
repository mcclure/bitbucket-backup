# Include this file for the platform compatibility library

import Math
import Macros
import glm
import unicode

when defined(js):
    include tsGlue # Stubs from dts2nim

    # Compatibility functions

    type
        glBuffer* = WebGLBuffer

    proc `or`*(a: float, b:float) : float =
        return (a.uint or b.uint).float

    const bufferNil* = nil
    const vertexAttribNil* = 0

    let cGL_FLOAT* = gl.FLOAT

    converter toArrayBufferView*(x:var Float32Array) : ArrayBufferView = return cast[ArrayBufferView](x)
    converter toArrayBufferView*(x:var Uint8Array) : ArrayBufferView = return cast[ArrayBufferView](x)

else:
    include sdlGlue # Imports sdl, opengl
    from random import nil

    # Hack so that gl.function gets rewritten as glFunction.
    # These macros are based on code provided by flyx on #nim IRC, reused with permission.
    # TODO: Long term it would be a good idea to switch to the nimx.portable_gl module.
    type VirtualModule {.pure.} = enum
        gl
    const gl* = VirtualModule.gl
    macro `.()`*(gl: VirtualModule, name: string, params: varargs[typed]): untyped =
        let glt = gl.getType()
        result = newCall($glt[0][0] & name.strVal())
        for param in params: result.add(param)

    # Untyped to work around a "known bug" with newIdentNode
    macro `.`*(gl: VirtualModule, name: string): untyped =
        let glt = gl.getType()
        result = newIdentNode(toUpper($glt[0][0]) & "_" & name.strVal())

    # Compatibility functions

    type
        Float32Array* = seq[float32]
        Uint8Array* = seq[uint8]
        glBuffer* = GLuint
        WebGLShader* = GLuint
        WebGLProgram* = GLuint
        WebGLUniformLocation* = GLint

    converter toGLsizei*(x:int) : GLsizei = return x.GLsizei
    converter toGLint*(x:int) : GLint = return x.GLint
    converter toGLuint*(x:int) : GLuint = return x.GLuint
    converter toGLuint*(x:GLint) : GLuint = return x.GLuint
    converter toGLsizeiptr*(x:int) : GLsizeiptr = return x.GLsizeiptr

    converter toPointer*(x:var Float32Array) : pointer = return x[0].addr
    converter toPointer*(x:var Float32Array) : ptr GLfloat = return x[0].addr
    converter toPointer*(x:var Uint8Array) : pointer = return x[0].addr
    converter toPointer*(x:var Uint8Array) : ptr uint8 = return x[0].addr

    proc newUint8Array*(size = 0) : Uint8Array =
        return newSeq[uint8](size)

    proc newFloat32Array*(size = 0) : Float32Array =
        return newSeq[float32](size)

    template number*[T](x:T) : T = x

    proc glCreateBuffer*() : glBuffer =
        var buffer: glBuffer
        glGenBuffers(1, addr buffer)
        return buffer

    proc glShaderSource*(shader: GLuint, source: string) =
        var
            sourceArray = [source]
            lengthArray = [source.len.GLint]
            sourceArrayAlloc = sourceArray.allocCStringArray
        glShaderSource(shader, 1.GLsizei, sourceArrayAlloc, lengthArray[0].addr)
        deallocCStringArray(sourceArrayAlloc)

    template glUniformMatrix4fv*(location: WebGLUniformLocation, transpose: bool, value: Float32Array) =
        glUniformMatrix4fv(uniform, 1, false, value)

    template glBufferData*[T](target: GLenum, ary: T, usage: GLenum) =
        glBufferData(target, ary.len*sizeof(float), ary, usage)

    proc checkShader*(shader: WebGLShader) : void =
        discard

    proc checkProgram*(program: WebGLProgram) : void =
        discard

    template MathRandom*() : float = random.random(1.0)

    template consoleLog*(x) = echo x

    const bufferNil* : GLuint = 0

    const vertexAttribNil* = nil

    const haveTone* = false
    proc soundState*(a,b,c,d:float) = discard

# Misc utility

proc radians*[T](degree:T):T=
    return degree/180*PI

var tempMatrix4 = newFloat32Array(16)
proc glmUniform4*(uniform:WebGLUniformLocation, mat:Mat4x4[float]) =
    for y in 0..<4:
        for x in 0..<4:
            tempMatrix4[ (y*4+x) ] = mat[y][x]
    gl.uniformMatrix4fv(uniform, false, tempMatrix4)