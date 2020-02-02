x = ScreenLabel("THE GAME WAS A METAPHOR FOR",24,"Wide")
x:setPosition(surface_width/5,surface_height/4-line_height)
screen():addChild(x)
x = ScreenLabel("THE STRUGGLE FOR DEMOCRACY",24,"Wide")
x:setPosition(surface_width/5,surface_height/4)
screen():addChild(x)
x = ScreenLabel("IN POST-MURABAK EGYPT",24,"Wide")
x:setPosition(surface_width/5,surface_height/4+line_height)
screen():addChild(x)

x = ScreenLabel("PRESS ESC",24,"Wide")
x:setPosition(surface_width/5,3*surface_height/4)
screen():addChild(x)

snd_die = Sound("media/die.ogg")
snd_die:setVolume(0.5)
snd_die:Play(false)
pitch = 1