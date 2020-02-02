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

function float32Set(a:Float32Array, k:number, v:number) {
  a[k] = v
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
        alert("WebGL is not working. Your browser may not support it.");
        gl = null
    }
  
    return gl
}

let gl = initWebGL(canvas)

let drawCanvasCallback : () => void

function resizeCanvas() {
    canvas.width = window.innerWidth
    canvas.height = window.innerHeight

    // Changing size will clear canvas
    drawCanvasCallback()
}

function animationLoop() {
    drawCanvasCallback()
    requestAnimationFrame(animationLoop)
}

function startup() {
    resizeCanvas()
    window.addEventListener('resize', resizeCanvas, false)
    requestAnimationFrame(animationLoop)
}