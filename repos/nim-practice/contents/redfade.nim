# Pulsating red background, print debug output on events

import sdl2
import mainloop

onEvent = proc(evt:Event) =
    case evt.kind
    of KeyDown:
        var evt = cast[KeyboardEventPtr](unsafeAddr(evt))
        echo("EVENT " & $evt.kind & ": " & $sdl2.getScancodeName(evt.keysym.scancode))
    of KeyUp:
        var evt = cast[KeyboardEventPtr](unsafeAddr(evt))
        echo("EVENT " & $evt.kind & ": " & $sdl2.getScancodeName(evt.keysym.scancode))
    else:
        echo("EVENT " & $evt.kind)

onDraw = proc() =
    render.setDrawColor( uint8(frame mod 256), 0, 0, 255 )
    render.clear

run()