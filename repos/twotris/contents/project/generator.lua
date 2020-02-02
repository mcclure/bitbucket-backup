-- Additional generators. Build level maps

class.ImageGenerator(Generator)

function ImageGenerator:_init(spec) self:super(spec) end

function ImageGenerator:build(w,h)
	return MapLoad(self.path)
end

class.GrayGenerator(Generator)

function GrayGenerator:_init(spec) self:super(spec) end

function GrayGenerator:build(w,h)
	local map = Generator.build(self,w,h)
	for x=1,w do
		for y=1,h do
			local color = engine.color_gray(math.random(0x10,0x20))
			project.set_color(map, x, y, color)
		end
	end
	return map
end

generators = {
	GrayGenerator()
}