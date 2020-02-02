# Application goes here

import glue
import math
import glm
import strutils

# What to draw

const square = [
    -1.0f, 1.0f, -1.0f,-1.0f,  1.0f,-1.0f,
     1.0f, 1.0f, -1.0f, 1.0f,  1.0f,-1.0f
]

const maxFloaters = 4

proc makeOffsetString() : string =
    return if optionRandom: $((MathRandom()-0.5)/100) else: "-0.001"
let offsetStringX = makeOffsetString()
let offsetStringY = makeOffsetString()

# Shaders

let vertexShaderCode = """
#define offsetX $#
#define offsetY $#
precision mediump float;
attribute vec2 position;
uniform vec2 zoom;
varying vec2 xy;
void main(void) {
    xy = position * zoom + vec2(offsetX, offsetY);
    gl_Position = vec4(position, 0.0, 1.0);
}
""".format(offsetStringX, offsetStringY)

let fragmentShaderCode = """
#define maxFloaters $#
precision mediump float;
varying vec2 xy;
uniform vec2 floaters[maxFloaters];
float ball(vec2 at) {
    vec2 base = at - xy;
    return 1.0/dot(base * base, vec2(1.0));
}
void main(void) {
    float dist = 0.0;
    for(int i = 0; i < maxFloaters; i++)
        dist += ball(floaters[i]);
    dist = abs(mod(dist, 1.0) * 2.0 - 1.0);
    dist = smoothstep(0.45, 0.55, dist);
    gl_FragColor = vec4(dist, dist, dist, 1.0);
}
""".format(maxFloaters)

# Create a model
var packedModel = newFloat32Array(square.len)
var vertices = gl.createBuffer()
for i in 0..<square.len:
    packedModel[i] = square[i]

# Create vertex shader
var vertexShader = gl.createShader(gl.VERTEX_SHADER)
gl.shaderSource(vertexShader, vertexShaderCode)
gl.compileShader(vertexShader)
checkShader(vertexShader)

# Create fragment shader
var fragmentShader = gl.createShader(gl.FRAGMENT_SHADER)
gl.shaderSource(fragmentShader, fragmentShaderCode)
gl.compileShader(fragmentShader)
checkShader(fragmentShader)

# Link program
var program = gl.createProgram()
gl.attachShader(program, vertexShader)
gl.attachShader(program, fragmentShader)
gl.linkProgram(program)
checkProgram(program)

var positionAttrib = gl.getAttribLocation(program, "position")
var zoomUniform = gl.getUniformLocation(program, "zoom")
var floatersUniform = gl.getUniformLocation(program, "floaters")

# State

let ent = newStandardEnt()

type
    GamePhase = ref object
        length:int
        points: seq[ Vec2[float] ]
    ActorState = ref object
        at: Vec2[float]
    ActorPoint = Vec2[float]

proc newGamePhase(length:int, points: seq[ActorPoint]) : GamePhase =
    return GamePhase(length:length, points:points)

proc newActorState(tempAt: ActorPoint) : ActorState =
    let at = tempAt
    return ActorState(at:at)

let gamePhases = [newGamePhase(0, @[vec2(0.0, 0.0), vec2(0.0, 0.0), vec2(0.0, 0.0)])]

# Mutable state
var
    phase {.exportc.} = 0
    phaseTick {.exportc.} = 0
    globalTick {.exportc.} = 0
    phaseState {.exportc.}: seq[ActorState] = nil
    pointsBuffer: Float32Array

proc activatePhase(idx: int) {.exportc.} =
    phase = idx
    phaseTick = 0
    phaseState = @[]
    let points = gamePhases[phase].points 
    for i in points:
        phaseState.add(newActorState(i))
    pointsBuffer = newFloat32Array(points.len*2)

activatePhase(0)

var ratio = vec2(1.0, 1.0)

# Debug

# Logic

proc makePeriod(default:float) : float =
    if optionRandom:
        return 1.0 + MathRandom()*0.3
    else:
        return default
let period1 = makePeriod(1.112)
let period2 = makePeriod(1.223)

gl.disable(gl.DEPTH_TEST)

# Upload values for model
gl.bindBuffer(gl.ARRAY_BUFFER, vertices)
gl.bufferData(gl.ARRAY_BUFFER, packedModel, gl.STATIC_DRAW)
gl.bindBuffer(gl.ARRAY_BUFFER, bufferNil)

ent.onResize = proc () =
    if displayWidth > displayHeight:
        ratio = vec2(displayWidth/displayHeight, 1.0)
    else:
        ratio = vec2(1.0,displayHeight/displayWidth)

    gl.useProgram(program)

ent.onDraw = proc () =
    # Set display properties and clear screen
    gl.viewport(0, 0, displayWidth, displayHeight)
    
    let gamePhase = gamePhases[phase]
    for i in 0..<phaseState.len:
        let actor = phaseState[i]
        pointsBuffer[i*2  ] = actor.at.x
        pointsBuffer[i*2+1] = actor.at.y

    let offsetTick = if optionStill: 0 else: max(phaseTick-2050, 0)
    let zoomAmount = pow(0.999, offsetTick.float)
    var closestDistance = 0.0

    # Set up draw
    gl.useProgram(program)
    gl.bindBuffer(gl.ARRAY_BUFFER, vertices)
    gl.vertexAttribPointer(positionAttrib, 2, cGL_FLOAT, false, 0, vertexAttribNil)
    gl.enableVertexAttribArray(positionAttrib)
    gl.uniform2fv(floatersUniform, pointsBuffer)
    gl.uniform2f(zoomUniform, ratio.x * zoomAmount, ratio.y * zoomAmount)
    gl.drawArrays(gl.TRIANGLES, 0, square.len div 2)

    # "Audio"
    if haveTone:
        discard # TODO

    # Update
    for i in 1..<phaseState.len:
        let actor = phaseState[i]
        actor.at.x = sin(phaseTick.float * PI*2/512.0 * pow(period1, i.float))
        actor.at.y = sin(phaseTick.float * PI*2/512.0 * pow(period2, i.float))
        let dist = actor.at.length
        if i == 1 or dist < closestDistance:
            closestDistance = dist

    # Adjust phase, if needed
    phaseTick += 1
    globalTick += 1
    if gamePhase.length > 0:
        if phaseTick >= gamePhase.length:
            activatePhase( (phase + 1) mod gamePhases.len )

    soundState(closestDistance, phaseTick)

# Init

startup(ent)
