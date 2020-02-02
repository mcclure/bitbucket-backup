# Application goes here

import glue
import math
import glm

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

gl.cullFace(gl.FRONT_AND_BACK)

# Create a model
var packedModel = newFloat32Array(cube.len)
var vertices = gl.createBuffer()
for i in 0..<cube.len:
    packedModel[i] = cube[i]

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
var colorUniform = gl.getUniformLocation(program, "color")
var mvpUniform = gl.getUniformLocation(program, "mvp")

# State

let ent = newStandardEnt()

let identity = mat4(1.0)
var externalCamera = lookAtLH( vec3(0.0, 4.0, 6.0), vec3(0.0, 0.0, 0.0), vec3(0.0, 1.0, 0.0) )
var externalProjection = identity

# Starting conditions for a cube
type CubeSpec = ref object of RootObj
    color: Vec3[float]
    theta: array[0..2, float]       # DEGREES, AXES Y X Z
    deltaTheta: array[0..2, float]  # SAME
    baseTransform: Mat4x4[float]

proc threeZeros() : array[3, float] =
    return [0.0, 0.0, 0.0]
proc newCubeSpec(color: Vec3[float], theta = threeZeros(), deltaTheta = threeZeros(), baseTransform = identity) : CubeSpec =
    return CubeSpec(color:color, theta:theta, deltaTheta:deltaTheta, baseTransform:baseTransform)

# Current status of a cube
type CubeState = ref object of RootObj
    active: bool
    theta: array[3, float]

proc newCubeState(cubeSpec: CubeSpec) : CubeState =
    return CubeState(active:true, theta:cubeSpec.theta)

# An animation phase, containing multiple cubes
type CubePhase = ref object of RootObj
    cubes: seq[CubeSpec]
    projection: Mat4x4[float]
    length: int
    chaotic: bool

proc cubePhaseProjection(fov:float) : Mat4x4[float] =
    return perspectiveLH( radians(fov), 16.0/9.0, 0.1, 100.0 )

proc newCubePhase(length:int, fov:float, cubes: seq[CubeSpec], chaotic = false) : CubePhase =
    return CubePhase(length:length, projection:cubePhaseProjection(fov), cubes:cubes, chaotic:chaotic)

# Given a slice scale, count sliceIn slices toward the center
proc inScale(sliceIn:int, sliceOf:int) : Mat4x4[float] =
    let xz = (sliceOf-sliceIn).float / sliceOf.float
    return scale(identity, vec3(xz, 1.0, xz))

# Phase map
var cubePhases {.exportc.} = [
    # Elevator roll
    newCubePhase(360, 45.0, @[
        newCubeSpec( vec3(0.0, 1.0, 1.0),  [0.0,  0.0,  0.0], [0.0,  0.0,  0.0] ),
        newCubeSpec( vec3(0.0, 0.0, 1.0),  [0.0,  0.0,  0.0], [0.0,  1.0,  0.0], inScale(2, 32) ) 
    ]),

    # Red emergence (long)
    newCubePhase(720, 45.0, @[
        newCubeSpec( vec3(1.0, 0.0, 0.0) ),
        newCubeSpec( vec3(0.0, 1.0, 1.0),  [0.0,  0.0,  0.0], [0.0,  1.0,  0.0], inScale(1, 32) ),
        newCubeSpec( vec3(0.0, 0.0, 1.0),  [0.0,  0.0,  0.0], [0.0, -0.5,  0.0], inScale(2, 32) ) 
    ]),
    # Elevator roll (reverse)
    newCubePhase(360, 45.0, @[
        newCubeSpec( vec3(0.0, 1.0, 1.0),  [0.0,  0.0,  0.0], [0.0,  0.0,  0.0] ),
        newCubeSpec( vec3(0.0, 0.0, 1.0),  [0.0,  0.0,  0.0], [0.0, -1.0,  0.0], inScale(2, 32) )
    ]),

    # Hallway
    newCubePhase(360, 45.0, @[
        newCubeSpec( vec3(1.0, 0.0, 0.0) ),
        newCubeSpec( vec3(0.0, 1.0, 1.0),  [0.0,  0.0,  0.0], [0.0,  1.0,  0.0], inScale(1, 32) ),
        newCubeSpec( vec3(0.0, 0.0, 1.0),  [0.0,  0.0,  0.0], [0.25, 0.0,  0.0], inScale(2, 32) ) 
    ]),
    # Elevator roll (invert)
    newCubePhase(360, 45.0, @[
        newCubeSpec( vec3(0.0, 0.0, 1.0),  [0.0,  0.0,  0.0], [0.0,  0.0,  0.0] ),
        newCubeSpec( vec3(0.0, 1.0, 1.0),  [0.0,  0.0,  0.0], [0.0,  1.0,  0.0], inScale(2, 32) ) 
    ]),
    # Hallway (wide, long)
    newCubePhase(720, 90.0, @[
        newCubeSpec( vec3(1.0, 0.0, 0.0) ),
        newCubeSpec( vec3(0.0, 1.0, 1.0),  [0.0,  0.0,  0.0], [0.0,  1.0,  0.0], inScale(1, 32) ),
        newCubeSpec( vec3(0.0, 0.0, 1.0),  [0.0,  0.0,  0.0], [0.25, 0.0,  0.0], inScale(2, 32) ) 
    ]),

    # Red emergence (wide)
    newCubePhase(360, 90.0, @[
        newCubeSpec( vec3(1.0, 0.0, 0.0) ),
        newCubeSpec( vec3(0.0, 1.0, 1.0),  [0.0,  0.0,  0.0], [0.0,  1.0,  0.0], inScale(1, 32) ),
        newCubeSpec( vec3(0.0, 0.0, 1.0),  [0.0,  0.0,  0.0], [0.0, -0.5,  0.0], inScale(2, 32) ) 
    ]),
    # Wild yellow 1
    newCubePhase(720, 45.0, @[
        newCubeSpec( vec3(1.0, 0.0, 0.0) ),
        newCubeSpec( vec3(0.0, 1.0, 1.0),  [0.0,  0.0,  0.0], [0.0,  1.0,  0.0], inScale(1, 32) ),
        newCubeSpec( vec3(0.0, 0.0, 1.0),  [0.0,  0.0,  0.0], [0.0, -0.5,  0.0], inScale(2, 32) ),
        newCubeSpec( vec3(1.0, 1.0, 0.0),  [0.0,  0.0,  0.0], [0.25, 0.0,  0.0], inScale(2, 32) ) 
    ]),
    # Elevator roll (repeat)
    newCubePhase(360, 45.0, @[
        newCubeSpec( vec3(0.0, 1.0, 1.0),  [0.0,  0.0,  0.0], [0.0,  0.0,  0.0] ),
        newCubeSpec( vec3(0.0, 0.0, 1.0),  [0.0,  0.0,  0.0], [0.0,  1.0,  0.0], inScale(2, 32) ) 
    ]),

    # Wild yellow 2
    newCubePhase(720*2, 45.0, @[
        newCubeSpec( vec3(1.0, 0.0, 0.0) ),
        newCubeSpec( vec3(0.0, 1.0, 1.0),  [0.0,  0.0,  0.0], [0.0,  0.88,  0.0], inScale(1, 32) ),
        newCubeSpec( vec3(0.0, 0.0, 1.0),  [0.0,  0.0,  0.0], [0.0, -0.15,  0.2], inScale(2, 32) ),
        newCubeSpec( vec3(1.0, 1.0, 0.0),  [0.0,  0.0,  0.0], [0.35, 0.0,   0.0], inScale(2, 32) ) 
    ]),

    # Elevator roll (final)
    newCubePhase(720, 45.0, @[
        newCubeSpec( vec3(0.0, 1.0, 1.0),  [0.0,  0.0,  0.0], [0.0,  0.0,  0.0] ),
        newCubeSpec( vec3(0.0, 0.0, 1.0),  [0.0,  0.0,  0.0], [0.0,  1.0,  0.0], inScale(2, 32) ) 
    ]),

    # Break
    newCubePhase(45, 45.0, @[
    ]),

    # CHAOS
    newCubePhase(0, 45.0, @[
        newCubeSpec( vec3(1.0, 0.0, 0.0) ),
        newCubeSpec( vec3(0.0, 1.0, 1.0),  [0.0,  0.0,  0.0], [0.0,  0.0,  0.0], inScale(1, 32) ),
        newCubeSpec( vec3(0.0, 0.0, 1.0),  [0.0,  0.0,  0.0], [0.0,  0.0,  0.0], inScale(2, 32) ),
    ], true)
]

# Mutable state
var
    phase {.exportc.} = 0
    phaseTick {.exportc.} = 0
    globalTick {.exportc.} = 0
    phaseState {.exportc.}: seq[CubeState] = nil

proc activatePhase(idx: int) {.exportc.} =
    phase = idx
    phaseTick = 0
    phaseState = @[]
    for cubeSpec in cubePhases[phase].cubes:
        let cubeState = newCubeState(cubeSpec)
        phaseState.add( cubeState )
        
        # Apply chaos
        if cubePhases[phase].chaotic:
            for idx in 0..<3:
                cubeSpec.deltaTheta[idx] = MathRandom() * 2.0 - 1.0
                cubeState.theta[idx] = MathRandom() * 360.0

activatePhase(0)

let axes = [
    vec3(1.0, 0.0, 0.0),
    vec3(0.0, 1.0, 0.0),
    vec3(0.0, 0.0, 1.0)
]

# Audio state/tools

const audioRecording = false
const audioPlayback = true

# glReadPixels is slow so for frames with known colors I just use prerecorded values
# Following is the output of a previous run with audioRecording = true:
const audioPlaybackLog = [
    [0,0,4], [0,1,4], [0,2,4], [9,0,6], [21,1,6], [42,0,4], [49,2,6], [70,1,4], [82,2,4], [99,0,6], [111,1,6], [132,0,4], [139,2,6], [160,1,4], [172,2,4], [189,0,6], [201,1,6], [222,0,4], [229,2,6], [250,1,4], [262,2,4], [279,0,6], [291,1,6], [312,0,4], [319,2,6], [340,1,4], [352,2,4], [364,2,6], [401,1,1], [405,2,1], [430,0,6], [436,1,6], [443,2,4], [451,0,4], [451,2,6], [458,0,1], [465,1,1], [471,2,4], [496,0,6], [500,1,4], [537,0,4], [544,2,6], [581,1,1], [585,2,1], [610,0,6], [616,1,6], [623,2,4], [631,0,4], [631,2,6], [638,0,1], [645,1,1], [651,2,4], [676,0,6], [680,1,4], [717,0,4], [724,2,6], [761,1,1], [765,2,1], [790,0,6], [796,1,6], [803,2,4], [811,0,4], [811,2,6], [818,0,1], [825,1,1], [831,2,4], [856,0,6], [860,1,4], [897,0,4], [904,2,6], [941,1,1], [945,2,1], [970,0,6], [976,1,6], [983,2,4], [991,0,4], [991,2,6], [998,0,1], [1005,1,1], [1011,2,4], [1036,0,6], [1040,1,4], [1077,0,4], [1089,2,6], [1101,1,6], [1122,2,4], [1129,0,6], [1150,1,4], [1162,0,4], [1179,2,6], [1191,1,6], [1212,2,4], [1219,0,6], [1240,1,4], [1252,0,4], [1269,2,6], [1281,1,6], [1302,2,4], [1309,0,6], [1330,1,4], [1342,0,4], [1359,2,6], [1371,1,6], [1392,2,4], [1399,0,6], [1420,1,4], [1432,0,4], [1446,2,6], [1478,2,4], [1491,0,6], [1518,1,6], [1522,2,1], [1526,2,6], [1535,0,1], [1545,1,1], [1575,2,1], [1576,0,6], [1606,1,6], [1616,2,6], [1625,0,1], [1635,1,1], [1665,2,1], [1666,0,6], [1696,1,6], [1706,2,6], [1715,0,1], [1725,1,1], [1755,2,1], [1756,0,6], [1786,1,6], [1796,2,6], [1809,0,4], [1821,1,4], [1842,0,6], [1849,2,4], [1870,1,6], [1882,2,6], [1899,0,4], [1911,1,4], [1932,0,6], [1939,2,4], [1960,1,6], [1972,2,6], [1989,0,4], [2001,1,4], [2022,0,6], [2029,2,4], [2050,1,6], [2062,2,6], [2079,0,4], [2091,1,4], [2112,0,6], [2119,2,4], [2140,1,6], [2152,2,6], [2160,0,4], [2160,1,4], [2160,2,4], [2163,2,6], [2169,0,6], [2238,1,6], [2246,2,1], [2249,2,6], [2252,0,1], [2255,0,6], [2265,1,1], [2326,1,6], [2336,2,1], [2339,2,6], [2342,0,1], [2345,0,6], [2355,1,1], [2416,1,6], [2426,2,1], [2429,2,6], [2432,0,1], [2435,0,6], [2445,1,1], [2506,1,6], [2516,2,1], [2519,2,6], [2522,0,1], [2525,0,6], [2535,1,1], [2596,1,6], [2606,2,1], [2609,2,6], [2612,0,1], [2615,0,6], [2625,1,1], [2686,1,6], [2696,2,1], [2699,2,6], [2702,0,1], [2705,0,6], [2715,1,1], [2776,1,6], [2786,2,1], [2789,2,6], [2792,0,1], [2795,0,6], [2803,1,4], [2872,2,4], [2878,0,4], [2882,2,6], [2901,0,6], [2921,1,1], [2933,0,4], [2936,2,4], [2956,1,6], [2985,1,1], [3004,0,6], [3008,2,6], [3020,1,4], [3041,2,4], [3059,0,4], [3062,2,6], [3081,0,6], [3101,1,1], [3113,0,4], [3116,2,4], [3136,1,6], [3165,1,1], [3184,0,6], [3188,2,6], [3200,1,4], [3221,2,4], [3239,0,4], [3240,0,3], [3240,1,3], [3240,2,3], [3241,0,4], [3246,2,6], [3278,2,3], [3310,0,6], [3318,1,6], [3322,2,1], [3323,2,4], [3331,0,4], [3331,2,6], [3338,0,1], [3345,1,1], [3351,2,4], [3376,0,6], [3380,1,4], [3417,0,4], [3424,2,6], [3461,1,1], [3465,2,1], [3490,0,6], [3496,1,6], [3503,2,4], [3511,0,4], [3511,2,6], [3518,0,1], [3525,1,1], [3531,2,4], [3556,0,6], [3560,1,4], [3597,0,4], [3604,2,6], [3641,1,1], [3645,2,1], [3670,0,6], [3676,1,6], [3683,2,4], [3691,0,4], [3691,2,6], [3698,0,1], [3705,1,1], [3711,2,4], [3736,0,6], [3740,1,4], [3777,0,4], [3784,2,6], [3821,1,1], [3825,2,1], [3850,0,6], [3856,1,6], [3863,2,4], [3871,0,4], [3871,2,6], [3878,0,1], [3879,0,3], [3883,1,3], [3891,2,4], [3923,0,2], [3924,0,6], [3954,0,3], [3960,0,4], [3960,1,4], [3969,0,6], [3981,1,6], [4002,0,4], [4009,2,6], [4030,1,4], [4042,2,4], [4059,0,6], [4071,1,6], [4092,0,4], [4099,2,6], [4120,1,4], [4132,2,4], [4149,0,6], [4161,1,6], [4182,0,4], [4189,2,6], [4210,1,4], [4222,2,4], [4239,0,6], [4251,1,6], [4272,0,4], [4279,2,6], [4300,1,4], [4312,2,4], [4320,0,3], [4320,1,3], [4320,2,3], [4321,0,4], [4321,1,4], [4327,2,6], [4368,2,3], [4379,2,1], [4418,2,6], [4419,1,6], [4432,1,4], [4456,1,1], [4474,2,1], [4497,0,6], [4509,1,6], [4514,0,4], [4520,2,6], [4541,1,1], [4570,0,1], [4576,0,6], [4576,2,1], [4577,1,3], [4577,2,3], [4578,1,1], [4578,2,1], [4611,1,6], [4622,2,6], [4632,0,1], [4644,1,1], [4678,2,1], [4679,0,6], [4713,1,6], [4725,2,6], [4735,0,1], [4746,1,1], [4777,0,3], [4777,1,3], [4780,2,3], [4783,0,6], [4825,0,3], [4839,2,6], [4844,0,4], [4860,1,4], [4876,2,4], [4897,0,6], [4928,0,4], [4944,2,6], [4967,2,4], [4986,0,6], [5028,1,6], [5041,0,1], [5052,1,4], [5056,1,1], [5088,0,6], [5122,1,6], [5144,0,1], [5154,2,6], [5155,1,1], [5175,2,4], [5190,0,6], [5225,1,6], [5237,0,4], [5237,2,6], [5257,1,1], [5291,1,3], [5292,2,3], [5350,2,6], [5389,2,3], [5401,1,4], [5407,2,1], [5440,2,6], [5489,2,4], [5510,0,6], [5544,0,4], [5557,2,6], [5586,2,4], [5603,0,6], [5639,1,6], [5648,2,6], [5655,0,1], [5666,1,1], [5701,0,6], [5701,2,1], [5736,1,6], [5747,2,6], [5757,0,1], [5760,0,4], [5760,1,4], [5760,2,4], [5769,0,6], [5781,1,6], [5802,0,4], [5809,2,6], [5830,1,4], [5842,2,4], [5859,0,6], [5871,1,6], [5892,0,4], [5899,2,6], [5920,1,4], [5932,2,4], [5949,0,6], [5961,1,6], [5982,0,4], [5989,2,6], [6010,1,4], [6022,2,4], [6039,0,6], [6051,1,6], [6072,0,4], [6079,2,6], [6100,1,4], [6112,2,4], [6129,0,6], [6141,1,6], [6162,0,4], [6169,2,6], [6190,1,4], [6202,2,4], [6219,0,6], [6231,1,6], [6252,0,4], [6259,2,6], [6280,1,4], [6292,2,4], [6309,0,6], [6321,1,6], [6342,0,4], [6349,2,6], [6370,1,4], [6382,2,4], [6399,0,6], [6411,1,6], [6432,0,4], [6439,2,6], [6460,1,4], [6472,2,4], [6480,0,0], [6480,1,0], [6480,2,0], 
]
var audioPlaybackPosition = 0

var rgbReadback {.exportc.} = newUint8Array(4)

proc bitmaskFromColor(x: Uint8Array) : int =
    var sum = 0
    for bit in 0..<3:
        if x[bit].int > 128:
            sum += (1 shl bit)
    return sum

var audioRecordLast = [-1,-1,-1]
var audioLog = ""
proc audioRecord(idx:int, colorCode:int) =
    if audioRecordLast[idx] != colorCode:
        audioLog &= ("[" & $globalTick & "," & $idx & "," & $colorCode & "], ")
        audioRecordLast[idx] = colorCode
proc audioRecordHalt() =
    consoleLog(audioLog)

# Debug

var debugCamera = false
proc debugMoveCamera() {.exportc.} = debugCamera = not debugCamera

# Logic

ent.onResize = proc () =
    externalProjection = perspectiveLH( radians(90.0), displayWidth / displayHeight, 0.1, 100.0 ) * externalCamera

ent.onDraw = proc () =
    # Upload values for model
    gl.bindBuffer(gl.ARRAY_BUFFER, vertices)
    gl.bufferData(gl.ARRAY_BUFFER, packedModel, gl.DYNAMIC_DRAW)
    gl.bindBuffer(gl.ARRAY_BUFFER, bufferNil)

    # Set display properties and clear screen
    gl.viewport(0, 0, displayWidth, displayHeight)
    gl.clearColor(0.0, 0.0, 0.0, 1.0);
    gl.enable(gl.DEPTH_TEST)
    gl.depthFunc(gl.LEQUAL) # Near things obscure far things
    gl.clear(gl.COLOR_BUFFER_BIT or gl.DEPTH_BUFFER_BIT)

    # Set up draw
    gl.useProgram(program)
    gl.bindBuffer(gl.ARRAY_BUFFER, vertices)
    gl.vertexAttribPointer(positionAttrib, 3, cGL_FLOAT, false, 0, vertexAttribNil)
    gl.enableVertexAttribArray(positionAttrib)

    let cubePhase = cubePhases[phase]
    let camera = if debugCamera: externalProjection else: cubePhase.projection

    for idx in 0..<cubePhase.cubes.len:
        let cubeState = phaseState[idx]
        if not cubeState.active:
            continue
        let cubeSpec = cubePhase.cubes[idx]

        # Draw and update state
        var modelTransform = cubeSpec.baseTransform
        for axis in 0..<3:
            let theta = cubeState.theta[axis]
            if theta != 0:
                modelTransform = rotate(identity, axes[axis], radians(theta)) * modelTransform
            cubeState.theta[axis] = (theta + cubeSpec.deltaTheta[axis]) mod 360.0
        gl.uniform3f(colorUniform, cubeSpec.color[0], cubeSpec.color[1], cubeSpec.color[2]);
        glmUniform4(mvpUniform, camera * modelTransform)
        gl.drawArrays(gl.TRIANGLES, 0, cube.len div 3)

    # "Audio"
    if haveTone:
        var samples:array[3, int]
        if audioPlayback and not cubePhase.chaotic:
            while audioPlaybackPosition < audioPlaybackLog.len and audioPlaybackLog[audioPlaybackPosition][0] <= globalTick:
                let point = audioPlaybackLog[audioPlaybackPosition]
                audioRecordLast[point[1]] = point[2]
                audioPlaybackPosition += 1
            samples = audioRecordLast
        else:
            for idx in 0..<3:
                gl.readPixels( ((idx+1)*displayWidth.int div 4) , (displayHeight.int div 2), 1, 1, gl.RGBA, gl.UNSIGNED_BYTE, rgbReadback)
                let colorCode = bitmaskFromColor(rgbReadback)
                samples[idx] = colorCode
                if audioRecording and not cubePhase.chaotic:
                    audioRecord(idx, colorCode)
        # Possible numbers: 1 3 4 6
        soundState(samples[0].float, samples[1].float, samples[2].float,
            if cubePhase.chaotic: 1.0 - phaseTick/(720*4) else: 0.0)

    # Adjust phase, if needed
    phaseTick += 1
    globalTick += 1
    if cubePhase.length > 0:
        if phaseTick >= cubePhase.length:
            activatePhase( (phase + 1) mod cubePhases.len )
            if audioRecording and cubePhases[phase].chaotic:
                audioRecordHalt()
    if cubePhase.chaotic and phaseTick > 720:
        cubePhase.projection = cubePhaseProjection((phaseTick.float - 720)/64.0 + 45)

# Init

startup(ent)
