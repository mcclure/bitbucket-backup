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

// Options

function emptyMap() : {} { return Object.create(null) }
let options = emptyMap()
if (window.location.hash) {
    let kvs = window.location.hash.substring(1).split('&')
    for (let kv of kvs) {
        let equalsAt = kv.indexOf('=')
        if (equalsAt != -1) {
            let key =   kv.substring(0, equalsAt)
            let value = kv.substring(equalsAt + 1)
            options[key] = value
        } else {
            options[kv] = null
        }
    }
}

let optionRandom = 'random' in options
let optionStill = 'still' in options

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
let noise
let lowpass
let bitcrushgain
let bitcrush
let lowcrush
let noise2
let startme = []
let oscStarted = false
let squareStarted = false
let semitone = Math.pow(2, 1/12)
let debugAudioHalt = false
function semitoneStep(by:number) : number {
    return Math.pow(semitone, by)
}

if (haveTone) {
    //bitcrush = new Tone.BitCrusher(8).toMaster()
    lowpass = new Tone.Filter(880, "lowpass", -12).toMaster();
    noise = new Tone.Noise({
                "volume" : -6,
                "type" : "white"
        }).connect(lowpass);
    startme.push(noise)
    
    bitcrushgain = new Tone.Volume(0).toMaster()
    bitcrush = new Tone.BitCrusher(1).connect(bitcrushgain)
    bitcrush.set({wet:0})
    lowcrush = new Tone.Filter(14080, "lowpass", -12).connect(bitcrush)
    noise2 = new Tone.Noise({"volume" : -56, "type" : "white"}).connect(lowcrush)
}

const transitionStart = 2050
const transitionHiss = 8000
const transitionRoar = 14000
const transitionScream = 22500
const transitionFadeoutStart = 18000
const transitionFadeoutEnd = 22000
const frequencyHiss = 14080
const frequencyScream = 110
const frequencyWaveHigh = 880
const frequencyWaveLow = 440
const volumeNoiseStart = -6
const volumeNoiseEnd = 0
const volumeBitcrushStart = 0
const volumeBitcrushEnd = -10

function soundState(f1:number, f2:number) {
    if (!oscStarted) {
        for(let item of startme)
            item.start()
        oscStarted = true
    }
    lowpass.set({frequency:frequencyWaveHigh - frequencyWaveLow*f1})

    if (debugAudioHalt)
            return

    if (f2 >= transitionStart) {
        if (!squareStarted) {
            noise2.start()
            squareStarted = true
        }
        noise2.set({volume: -56 + Math.min((f2 - transitionStart)/transitionHiss, 1)*50})
        if (f2 >= transitionHiss) {
            bitcrush.set({wet: Math.min((f2-transitionHiss)/(transitionRoar-transitionHiss), 1)})
            lowcrush.set({frequency: frequencyHiss -
                Math.min( (f2 - transitionHiss)/(transitionScream - transitionHiss), 1 ) * (frequencyHiss-frequencyScream)})
        }
        if (f2 >= transitionFadeoutStart) {
            let progress = (f2-transitionFadeoutStart)/(transitionFadeoutEnd-transitionFadeoutStart)
            bitcrushgain.set({volume:volumeBitcrushStart + (volumeBitcrushEnd-volumeBitcrushStart)*progress})
            noise.set({volume:volumeNoiseStart + (volumeNoiseEnd-volumeNoiseStart)*progress})
        }
    }
}
