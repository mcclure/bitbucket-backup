-- Additional Things. On-screne actors

-- Util functions

function game_playsample_set() end -- I DON'T CARE

-- Actual things

function tableToGrid(t, w, h)
	w = math.floor(w/2)+1
	h = math.floor(h/2)+1
	local g = SparseGrid()
	for y,row in ipairs(t) do
		for x,cell in ipairs(row) do
			g:set(x-w, y-h, cell)
		end
	end
	return g
end

class.Player(Thing)

function Player:_init(spec)
	pull(self, {
		down = {
		},
		pressed = {
		},
		mutator = tableToGrid({{-0.45,0.45,-0.45},{0.25,1,0.25},{-0.05,0.05,-0.05}}, 3, 3),
	})
	self:super(spec)
end

function Player:mod(stage, key)
	local source = stage[key]
	local target = self.swap
	local w,h = source.w, source.h
	if not target then
		target = MapMake(w, h)
	end

	local channel = 0
	for y=0,(h-1) do
		for x=0,(w-1) do
			local accumulate = 0
			for d, v in self.mutator:iter() do
				local current = project.get_color_x(source, channel, x+d.x, y+d.y)
				accumulate = accumulate + current*v
			end
			if accumulate < 0 then accumulate = 0 elseif accumulate > 255 then accumulate = 255 end
			project.set_color(target, x, y, engine.color_gray(accumulate))
		end
	end
	stage[key] = target
	self.swap = source
end

function Player:onTick()
	self:mod(self.stage, "base")
end