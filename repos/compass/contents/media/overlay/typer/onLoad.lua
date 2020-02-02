-- "Print out slowly" class

-- Adapted from ADP 1f42776ab657

class "Typer" (Ent)

function Typer:Typer(spec)
	local s = {printed = Queue(), tx = 0, ty = 0, dos = dos, speed=18}
	Ent.Ent(self, tableMerge(s, spec))
end

-- Setup DOS, shaders

function Typer:print(str) -- Preprint
	-- Word wrap -- Note: Messes up oddly if a line ends with a newline
	local word = ""
	local linelen = -1 -- This should (?) be 0, but if I set it to 0 things break. What?
	local havespace = false
	local haveany = false
	
	for c in str:gmatch(".") do
		if c == " " or c == "\r" or c == "\n" then
			wasspace = c == " "
			if linelen + #word >= 39 then
				if haveany and linelen < 39 then self.printed:push("\n") end
				wasspace = false
			elseif havespace then
				self.printed:push(" ")
				linelen = linelen + 1
			end
			if not wasspace then
				linelen = 0
				havespace = false
			else
				havespace = true
			end
			self.printed:push(word)
			linelen = linelen + #word
			word = ""
			haveany = true
			if not wasspace then
				self.printed:push(c)
			end
		else
			word = word .. c
		end
	end
	if #word then -- Special handling for final word. Could fold in with above...?
		if havespace then
			if linelen + #word >= 39 then
				self.printed:push("\n")
			else
				self.printed:push(" ")
			end
		end
		self.printed:push(word)
	end
end

function Typer:crlf() -- Postprint
	self.ty = self.ty + 1
	self.tx = 0
	if self.ty >= 24 then
		self.dos:scroll()
		self.ty = 23
	end
end

function Typer:splat(str)
	for c in str:gmatch(".") do
		if c == "\n" then
			self:crlf()
		else
			if c == "\012" then c = "\010" end -- I REALLY need to be able to print 010
			self.dos:set(self.tx,self.ty,c) -- Letter
			self.tx = self.tx + 1
			if self.tx >= 40 then self:crlf() end
		end
	end
end

function Typer:special(t) -- Override for special handling when table printed
end

function Typer:onTick()
	local grains = self.speed
	while 0 < self.printed.count and 0 < grains do
		local p = self.printed:peek()
		
		if "table" == type( p ) then
			self:special(p)
			self.printed:pop()
		else
			if #p <= grains then
				self.printed:pop()
			else
				self.printed[self.printed.low] = string.sub(p,grains+1)
				p = string.sub(p,1,grains)
			end
			
			self:splat(p)
			
			grains = grains - #p
		end
	end
end

-- Loose utilities

function scrub(s) -- String mangling
	local r = "" -- Remove all whitespace:
	for x in s:lower():gmatch("%a+") do r = r .. x end 
	return r
end
function string.starts(String,Start)
   return string.sub(String,1,string.len(Start))==Start
end
function sanitize(s)
	if s then return s else return "(nil)" end
end
