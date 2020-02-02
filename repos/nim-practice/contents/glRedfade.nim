# Pulsating red background, print debug output on events

import sdl2
import mainloop
import opengl

onInit = proc() =
    loadExtensions()
    glViewport( 0, 0, windowWidth.GLsizei, windowHeight.GLsizei )

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
    glClearColor((frame mod 256) / 256, 0.0, 0.0, 0.0)
    glClear(GL_COLOR_BUFFER_BIT or GL_DEPTH_BUFFER_BIT)

run(true)
