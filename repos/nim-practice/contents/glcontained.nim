# Must install sdl2, opengl, glm from nimble and place SDL2_gfx.dll in directory

import sdl2, sdl2/gfx
include opengl
import glm
import math

### OpenGL setup

discard sdl2.init(INIT_EVERYTHING)

const
    targetFps* = 60
    targetTpf* = int(1000/target_fps) ## "ticks per frame"

var
    window: WindowPtr
    context: GlContextPtr

    displayWidth*: int
    displayHeight*: int

    fpsman: FpsManager

fpsman.init
fpsman.setFramerate targetFps

discard glSetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2)
discard glSetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1)
discard glSetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE)
window = createWindow("SDL Skeleton", 100, 100, 640,480, SDL_WINDOW_OPENGL)
context = glCreateContext(window)
loadExtensions()
block:
    var cWindowWidth, cWindowHeight : cint = 0
    sdl2.getSize(window, cWindowWidth, cWindowHeight)
    displayWidth = cWindowWidth; displayHeight = cWindowHeight

# OpenGL helpers

type glBuffer* = GLuint
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

var tempMatrix4 = newSeq[float32](16)
proc glmUniform4*(uniform:GLsizei, mat:Mat4x4[float]) =
    for y in 0..<4:
        for x in 0..<4:
            tempMatrix4[ (y*4+x) ] = mat[y][x]
    gluniformMatrix4fv(uniform, 1.GLsizei, false, tempMatrix4[0].addr)

proc radians*[T](degree:T):T=
    return degree/180*PI

### Draw setup

# What to draw

const cube = [
    -1.0f,-1.0f,-1.0f, -1.0f,-1.0f, 1.0f, -1.0f, 1.0f, 1.0f,
     1.0f, 1.0f,-1.0f, -1.0f,-1.0f,-1.0f, -1.0f, 1.0f,-1.0f,
     1.0f,-1.0f, 1.0f, -1.0f,-1.0f,-1.0f,  1.0f,-1.0f,-1.0f,
     1.0f, 1.0f,-1.0f,  1.0f,-1.0f,-1.0f, -1.0f,-1.0f,-1.0f,
    -1.0f,-1.0f,-1.0f, -1.0f, 1.0f, 1.0f, -1.0f, 1.0f,-1.0f,
     1.0f,-1.0f, 1.0f, -1.0f,-1.0f, 1.0f, -1.0f,-1.0f,-1.0f,
    -1.0f, 1.0f, 1.0f, -1.0f,-1.0f, 1.0f,  1.0f,-1.0f, 1.0f,
     1.0f, 1.0f, 1.0f,  1.0f,-1.0f,-1.0f,  1.0f, 1.0f,-1.0f,
     1.0f,-1.0f,-1.0f,  1.0f, 1.0f, 1.0f,  1.0f,-1.0f, 1.0f,
     1.0f, 1.0f, 1.0f,  1.0f, 1.0f,-1.0f, -1.0f, 1.0f,-1.0f,
     1.0f, 1.0f, 1.0f, -1.0f, 1.0f,-1.0f, -1.0f, 1.0f, 1.0f,
     1.0f, 1.0f, 1.0f, -1.0f, 1.0f, 1.0f,  1.0f,-1.0f, 1.0f
]

# Shaders

let vertexShaderCode = """
precision mediump float;
attribute vec3 position;
uniform mat4 mvp;
void main(void) {
    gl_Position = mvp * vec4(position, 1.0);
}
"""

let fragmentShaderCode = """
precision mediump float;
uniform vec3 color;
void main(void) {
    gl_FragColor = vec4(color, 1.0);
}
"""

glcullFace(GL_FRONT_AND_BACK)

# Create a model
var packedModel = newSeq[float32](cube.len)
var vertices = glcreateBuffer()
for i in 0..<cube.len:
    packedModel[i] = cube[i]

# Create vertex shader
var vertexShader = glcreateShader(GL_VERTEX_SHADER)
glshaderSource(vertexShader, vertexShaderCode)
glcompileShader(vertexShader)

# Create fragment shader
var fragmentShader = glcreateShader(GL_FRAGMENT_SHADER)
glshaderSource(fragmentShader, fragmentShaderCode)
glcompileShader(fragmentShader)

# Link program
var program = glcreateProgram()
glattachShader(program, vertexShader)
glattachShader(program, fragmentShader)
gllinkProgram(program)

var positionAttrib = glgetAttribLocation(program, "position")
var colorUniform = glgetUniformLocation(program, "color")
var mvpUniform = glgetUniformLocation(program, "mvp")

var identity = mat4(1.0)
var externalCamera = lookAtLH( vec3(0.0, 4.0, 6.0), vec3(0.0, 0.0, 0.0), vec3(0.0, 1.0, 0.0) )
var externalProjection = perspectiveLH( radians(90.0), displayWidth / displayHeight, 0.1, 100.0 ) * externalCamera

###  Draw loop

var runGame = true
var modelTransform = mat4(1.0)

while runGame:
    var evt = sdl2.defaultEvent

    while pollEvent(evt):
        if evt.kind == QuitEvent:
            runGame = false
            break

    let tick = sdl2.getTicks()

    # DRAW HERE
    glbindBuffer(GL_ARRAY_BUFFER, vertices)
    glbufferData(GL_ARRAY_BUFFER, (packedModel.len*4).GLsizei, packedModel[0].addr, GL_DYNAMIC_DRAW)
    glbindBuffer(GL_ARRAY_BUFFER, 0)

    # Set display properties and clear screen
    glviewport(0.GLint, 0.GLint, displayWidth.GLsizei, displayHeight.GLsizei)
    glclearColor(0.0, 0.0, 0.0, 1.0);
    glenable(GL_DEPTH_TEST)
    gldepthFunc(GL_LEQUAL) # Near things obscure far things
    glclear(GL_COLOR_BUFFER_BIT or GL_DEPTH_BUFFER_BIT)

    # Set up draw
    gluseProgram(program)
    glbindBuffer(GL_ARRAY_BUFFER, vertices)
    glvertexAttribPointer(positionAttrib.GLuint, 3.GLint, cGL_FLOAT, false, 0.GLSizei, nil)
    glenableVertexAttribArray(positionAttrib.GLuint)

    modelTransform = rotate(identity, vec3(0.0, 1.0, 0.0), radians(1.0)) * modelTransform
    gluniform3f(colorUniform, 1.0, 1.0, 0.0);
    glmUniform4(mvpUniform, externalProjection * modelTransform)
    gldrawArrays(GL_TRIANGLES, 0.GLint, (cube.len div 3).GLSizei)

    # Done
    glSwapWindow(window)

    fpsman.delay

destroy window
