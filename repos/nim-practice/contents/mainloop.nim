## Core driver for an sdl program. Import, set callbacks, then call run()
import sdl2, sdl2/audio, sdl2/gfx

discard sdl2.init(INIT_EVERYTHING)

const
    targetFps* = 60
    targetTpf* = int(1000/target_fps) ## "ticks per frame"

    audioSampleRate* = 44100
    audioRequestBufferSizeInSamples* = 1024
    audioRequestBytesPerSample* = 2  # 16 bit PCM
    audioRequestBufferSizeInBytes* = audioRequestBufferSizeInSamples * audioRequestBytesPerSample

var
    window*: WindowPtr

    render*: RendererPtr     # 2D drawing
    context*: GlContextPtr # 3D drawing

    windowWidth*: int
    windowHeight*: int

    audioSpec*: AudioSpec
    fpsman: FpsManager

fpsman.init
fpsman.setFramerate targetFps

# Callbacks for program to overload
var
    onInit*   : proc()
    onUpdate* : proc(dt:int) ## In ms/"ticks"
    onDraw*   : proc()
    onEvent*  : proc(x:Event)
    onAudio*  : proc(userdata: pointer; stream: ptr uint8; len: cint) {.gcsafe, thread.}

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

## Start game
proc run*(openGl = false) =
    if openGl:
        discard glSetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2)
        discard glSetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1)
        discard glSetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE)
        window = createWindow("SDL Skeleton", 100, 100, 640,480, SDL_WINDOW_OPENGL)
        context = glCreateContext(window)
    else:
        window = createWindow("SDL Skeleton", 100, 100, 640,480, 0)
        render = createRenderer(window, -1, Renderer_Accelerated or Renderer_PresentVsync or Renderer_TargetTexture)

    var playingAudio = false

    block:
        var cWindowWidth, cWindowHeight : cint = 0
        sdl2.getSize(window, cWindowWidth, cWindowHeight)
        windowWidth = cWindowWidth; windowHeight = cWindowHeight

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

    if onInit != nil:
        onInit()

    while runGame:
        var evt = sdl2.defaultEvent

        while pollEvent(evt):
            if evt.kind == QuitEvent:
                runGame = false
                break
            if onEvent != nil:
                onEvent(evt)

        let tick = sdl2.getTicks()

        if onUpdate != nil:
            onUpdate( int(tick.int64 - lastFrameAt.int64) )

        if onDraw != nil:
            onDraw()

        if openGl:
            glSwapWindow(window)
        else:
            render.present

        lastFrameAt = tick
        frame = frame + 1
        fpsman.delay

    if render != nil:
        destroy render
    destroy window
    if playingAudio:
        pauseAudio(1)
        closeAudio()
