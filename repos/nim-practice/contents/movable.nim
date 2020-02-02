## Square moves around; chime object triggers more frequently when square is close

import sdl2
import mainloop
from math import sqrt, floor, round, sin, PI

var
    x, y, dx, dy, moveDx, moveDy, gemX, gemY, lastFireFrame: int
    maxDist: float
    up, down, left, right: bool

const
    maxSpeed = 5
    rectSize = 40
    maxPulse = targetFps
    pulseColorFall = (8 * 60 / targetFps).floor.int
    pulseColorMin = 64

proc square(x: float): float = return x * x
proc distance(x,y,x2,y2: float): float = return sqrt( square(x2 - x) + square(y2 - y) )
proc distance(x,y,x2,y2: int):   float = return distance(x.float, y.float, x2.float, y2.float)

proc castKeyEvent(evt:Event):KeyboardEventPtr =
    return cast[KeyboardEventPtr](unsafeAddr(evt))

onInit = proc() =
    x = windowWidth div 2
    y = windowHeight div 2
    gemX = (windowWidth div 4) * 3
    gemY = windowHeight div 6
    maxDist = distance(0, windowHeight, gemX, gemY)
    dx = 0; dy = 0; moveDx = 0; moveDy = 0; lastFireFrame = 0
    up = false; down = false; left = false; right = false

onEvent = proc(evt:Event) =
    case evt.kind
    of KeyDown:
        var evt = castKeyEvent(evt)
        case evt.keysym.scancode
        of SDL_SCANCODE_LEFT:
            left = true
            dx = -1
        of SDL_SCANCODE_RIGHT:
            right = true
            dx = 1
        of SDL_SCANCODE_UP:
            up = true
            dy = -1
        of SDL_SCANCODE_DOWN:
            down = true
            dy = 1
        else: discard
    of KeyUp:
        var evt = castKeyEvent(evt)
        case evt.keysym.scancode
        of SDL_SCANCODE_LEFT:
            left = false
            dx = if right: 1 else: 0
        of SDL_SCANCODE_RIGHT:
            right = false
            dx = if left: -1 else: 0
        of SDL_SCANCODE_UP:
            up = false
            dy = if down: 1 else: 0
        of SDL_SCANCODE_DOWN:
            down = false
            dy = if up: -1 else: 0
        else: discard
    else: discard

onUpdate = proc(dt:int) =
    let
        newDx = dx * maxSpeed
        newDy = dy * maxSpeed
    if newDx > moveDx: moveDx += 1 elif newDx < moveDx: moveDx -= 1
    if newDy > moveDy: moveDy += 1 elif newDy < moveDy: moveDy -= 1

    x += moveDx
    y += moveDy

    let pulseTime = distance(x, y, gemX, gemY) / maxDist * maxPulse * (dt / targetTpf)

    if frame - lastFireFrame > pulseTime.floor.int:
        lastFireFrame = frame

onDraw = proc() =
    render.setDrawColor( 32, 0, 0, 255 )
    render.clear

    # Player
    render.setDrawColor( 256-32, 0, 0, 255 )
    block:
        var rect = (x.cint, y.cint, rectSize.cint, rectSize.cint).Rect
        render.fillRect( rect )

    # Gem
    let shade = max( 255 - (frame - lastFireFrame) * pulseColorFall, pulseColorMin ).uint8
    render.setDrawColor( shade, shade, 0, 255 )
    block:
        var rect = (gemX.cint, gemY.cint, rectSize.cint, rectSize.cint).Rect
        render.fillRect( rect )

# Audio
block:
    const
        frequency = 180      # Hz
        volume = 0.1          # [0..1]
        volumeRamp = 0.0001
        speed = float(audioSampleRate) / float(frequency)
    var
        audioAt = 0
        audioLastFireAt = 0
        audioLastFireFrame = 0

    proc SineAmplitude(): int16 =
        let damp = 1 - float(audioAt - audioLastFireAt)*volumeRamp
        if damp < 0:
            return 0
        else:
            let value = (if sin(float(audioAt mod int(speed)) / speed * 2 * PI) > 0: 1 else: -1) * 32767 * volume * damp
            return int16(round( value ))

    onAudio = proc(userdata: pointer; stream: ptr uint8; len: cint) {.gcsafe, thread.} =
        if audioLastFireFrame != lastFireFrame:
            audioLastFireFrame = lastFireFrame
            audioLastFireAt = audioAt
        for i in 0..int16(audioSpec.samples)-1:
            cast[ptr int16](cast[int](stream) + i * audioRequestBytesPerSample)[] = SineAmplitude()
            audioAt += 1

run()