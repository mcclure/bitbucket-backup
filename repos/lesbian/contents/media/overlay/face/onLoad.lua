-- On Load
ticks = 0

stopall()
au.face:Play(false)

Services.Core:getServices().drawScreensFirst = true

local s = r(Screen())


s:setScreenShader("FilterMaterial")
shader = shaderBindings(s,
                       {aspect=(surface_height/surface_width), span=lm.pixel})

local i = ScreenImage("media/face.png")
print({i:getImageHeight(), surface_height})
local scale = surface_height/i:getImageHeight()
i:setPosition((surface_width-i:getImageWidth()*scale)/2,0)
i:setScale(scale,scale)

s:addEntity(i)

killDos()
dos = type_automaton()
dos:insert()

local messages = {
	"BETRAYAL! THE CASTLE IS REVOLT?",
	"OH NO!!!",
	"HALT! THE QUEEN IS TO RECOVER A SERVANT!",
	"MY OLD NEMESIS!!",
	"THE BEANS HAVE SPOILED!!",
	"ASH TAVES LABAI PASIILGAU!!",
	"EIKITE SU MANIMI!",
	"IT IS NOT POSSIBLE!!"
}
local message = messages[fm.level] or messages[math.random(#messages)]

dos:set_centered(0,23,40,message)