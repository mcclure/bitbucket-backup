## Driver for an SDL program. Equivalent of tsGlue.ts

import sdl2, sdl2/audio, sdl2/gfx
include opengl # To re-export

discard sdl2.init(INIT_EVERYTHING)

const
    targetFps* = 60
    targetTpf* = int(1000/target_fps) ## "ticks per frame"

    audioSampleRate* = 44100
    audioRequestBufferSizeInSamples* = 1024
    audioRequestBytesPerSample* = 2  # 16 bit PCM
    audioRequestBufferSizeInBytes* = audioRequestBufferSizeInSamples * audioRequestBytesPerSample

var
    window: WindowPtr
    context: GlContextPtr

    displayWidth*: int
    displayHeight*: int

    audioSpec*: AudioSpec
    fpsman: FpsManager

fpsman.init
fpsman.setFramerate targetFps

# Callbacks for program to overload
type StandardEnt* = ref object of RootObj
    onAdd*     : proc()
    onDismiss* : proc()
    onInit*    : proc()
    onResize*  : proc()
    onUpdate*  : proc(dt:int) ## In ms/"ticks"
    onDraw*    : proc()
    onEvent*   : proc(x:Event)

# Callback is outside of ent structure because it's separately threaded
var onAudio   : proc(userdata: pointer; stream: ptr uint8; len: cint) {.gcsafe, thread.}

proc newStandardEnt*() : StandardEnt =
    return StandardEnt(onAdd:nil, onDismiss:nil, onInit:nil, onResize:nil, onUpdate:nil, onDraw:nil, onEvent:nil)

var root* = newStandardEnt()

var
    runGame* = true          ## When false, game will quit
    lastFrameAt:uint32 = 0
    frame* = 0               ## Number of frames completed since startup
    didAudioGcInit = false

proc onAudioWrapper(userdata: pointer; stream: ptr uint8; len: cint) {.cdecl, gcsafe, thread.} =
    if not didAudioGcInit:
        setupForeignThreadGc()
        didAudioGcInit = true
    onAudio(userdata, stream, len)

## Early init (Do this at top level so I can be lazy and use gl stuff in the top level)
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

## Start game
proc startup*(ent : StandardEnt) =
    if (ent != nil):
        root = ent
        if root.onAdd != nil:
            root.onAdd()

    var playingAudio = false

    block audio:
        didAudioGcInit = false
        if onAudio != nil:
            var audioRequestSpec = AudioSpec(
                freq: audioSampleRate.cint,
                format: AUDIO_S16, # 16 bit PCM
                channels: 1,       # mono
                silence: 0,        # unused
                samples: audioRequestBufferSizeInBytes,
                padding: 0,
                callback: onAudioWrapper,
                userdata: nil
            )
            if openAudio(addr(audioRequestSpec), addr(audioSpec)) != 0:
                echo("Couldn't open audio device. " & $getError() & "\n")
                break audio
            echo("frequency: ", audioSpec.freq)
            echo("format: ", audioSpec.format)
            echo("channels: ", audioSpec.channels)
            echo("samples: ", audioSpec.samples)
            echo("padding: ", audioSpec.padding)
            if audioSpec.format != AUDIO_S16:
                echo("Couldn't open 16-bit audio channel.")
                break audio
            playingAudio = true
            pauseAudio(0)

    if root.onInit != nil:
        root.onInit()
    if root.onResize != nil:
        root.onResize()

    while runGame:
        var evt = sdl2.defaultEvent

        while pollEvent(evt):
            if evt.kind == QuitEvent:
                runGame = false
                break
            if root.onEvent != nil:
                root.onEvent(evt)

        let tick = sdl2.getTicks()

        if root.onUpdate != nil:
            root.onUpdate( int(tick.int64 - lastFrameAt.int64) )

        if root.onDraw != nil:
            root.onDraw()

        glSwapWindow(window)

        lastFrameAt = tick
        frame = frame + 1
        fpsman.delay

    destroy window
    if playingAudio:
        pauseAudio(1)
        closeAudio()

    if root.onDismiss != nil:
        root.onDismiss()