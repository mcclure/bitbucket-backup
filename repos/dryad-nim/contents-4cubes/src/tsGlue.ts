// This is converted into tsGlue.nim, then incorporated into glue.nim 

function testAssert(condition: boolean, message:string) {
    if (!condition)
        throw new Error("Assert failed: " + message)
}

function consoleLog(message: string) {
    console.log(message)
}

function consoleError(message: string) {
     console.error(message)
}

function checkShader(shader: WebGLShader) {
    var compiled = gl.getShaderParameter(shader, gl.COMPILE_STATUS);
    if (!compiled)
        console.error(gl.getShaderInfoLog(shader))
}

function checkProgram(program: WebGLProgram) {
    var linked = gl.getProgramParameter(program, gl.LINK_STATUS)
    if (!linked)
        console.error(gl.getProgramInfoLog(program))
}

// Entity

class Ent {
    children: Ent[]
    route(key: string, ...args:any[]) {
        if (this[key])
            this[key].apply(this, args)
        if (this.children)
            for (let child of this.children)
                child.route(key, args)
    }
    addChild(child: Ent) {
        if (!this.children)
            this.children = []
        this.children.push(child)
        child.route('onAdd', this)
    }
}

class StandardEnt extends Ent {
    onAdd : (parent: Ent) => void
    onDraw : () => void
    onResize : () => void
    onDismiss : () => void
}

let root : Ent = new Ent()

function setRoot(newRoot: Ent) {
    root.route('onDismiss')
    root = newRoot
    newRoot.route('onAdd', null)
}

// WebGL

let canvas = <HTMLCanvasElement> document.getElementById('canvas')

function initWebGL(canvas) : WebGLRenderingContext {
    let gl = null

    try {
        // Try to grab the standard context. If it fails, fallback to experimental.
        gl = <WebGLRenderingContext> (canvas.getContext("webgl") || canvas.getContext("experimental-webgl") )
    }
    catch(e) {}
  
    // If we don't have a GL context, give up now
    if (!gl) {
        gl = null
    }
  
    return gl
}

let gl = initWebGL(canvas)

// WebGL fallback
if (!gl) {
    let div = document.createElement('div')
    div.className = 'compatibility-error'
    let textNode = document.createTextNode("Sorry, but this page requires WebGL, and it looks like your browser doesn't support that.")
    div.appendChild(textNode)
    canvas.parentElement.replaceChild(div, canvas)
}

function drawCanvas() {
    root.route('onDraw')
}

let displayWidth = 0
let displayHeight = 0

function resizeCanvas() {
    canvas.width = displayWidth = window.innerWidth
    canvas.height = displayHeight = window.innerHeight
    root.route('onResize')

    // Changing size will clear canvas
    drawCanvas()
}

function animationLoop() {
    drawCanvas()
    requestAnimationFrame(animationLoop)
}

function startup(withRoot: Ent) {
    if (!gl)
        return
    if (withRoot)
        setRoot(withRoot)
    resizeCanvas()
    window.addEventListener('resize', resizeCanvas, false)
    requestAnimationFrame(animationLoop)
}

// Sound

declare var Tone
let haveTone = typeof Tone !== 'undefined'
let oscs = Array(3)
let squares = Array(4)
let oscStarted = false
let squareStarted = false
let semitone = Math.pow(2, 1/12)
function semitoneStep(by:number) : number {
    return Math.pow(semitone, by)
}

if (haveTone) {
    for(let idx = 0; idx < oscs.length; idx++) {
        oscs[idx] = new Tone.Oscillator({
                        "frequency" : 880,
                        "volume" : -14
                }).toMaster();
    }
    let steps = [-12, 0, 3, 24]
    for(let idx = 0; idx < squares.length; idx++) {
        squares[idx] = new Tone.Oscillator({
                        "frequency" : 220 * semitoneStep(steps[idx % steps.length]),
                        "volume" : -14,
                        "type" : "square"
                }).toMaster();
    }
}
function soundState(f1:number, f2:number, f3:number, squareVolume:number) {
    if (!oscStarted) {
        for(let osc of oscs)
            osc.start()
        oscStarted = true
    }
    oscs[0].set({frequency: 55                *(1 << f1), volume:-13.5-f1/2})
    oscs[1].set({frequency: 55*semitoneStep(4)*(1 << f2), volume:-13.5-f2/2})
    oscs[2].set({frequency: 55*semitoneStep(3)*(1 << f3), volume:-13.5-f3/2})
    if (squareVolume > 0) {
        if (!squareStarted) {
            for(let square of squares)
                square.start()
            squareStarted = true
        }
        let db = -14 - Math.pow(2, (1-squareVolume)*5) // I don't know how to work with db...?
        for(let square of squares)
            square.set({volume: db})
    } else if (squareStarted) {
        for(let square of squares)
            square.stop()
        squareStarted = false
    }
}
