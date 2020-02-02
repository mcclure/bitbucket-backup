class.CutTape() -- "Generators" but make sounds instead of images

function CutTape:_init(spec)
	pull(self,spec)
end

function CutTape:build()
	local result = project.playset_make()
	self.play = self.play or SampleCache(self.path)
	self:cuts()
	
	local sampleset = project.make_sampleset(self.seg, self.seglen)
	project.playset_set(result, sampleset, 1, 0,0,0,0,0)
	return result
end

function CutTape:cuts()
	local seg = {}
	
	local start = 0
	local size = 0
	
	for i=0,(self.play.len-1) do
		local sample = self.play.data[i]
		size = size + 1
		if size >= self.span then
			local cropped = project.crop(self.play, start, size)
			table.insert(seg, cropped)
			start = i+1
			size = 0
		end
	end
	
	local memo = {}
	
	local function sortValue(a)
		local total = memo[a]
		total = 0
		for i=0,(a.len-1) do
			local sample = a.data[i]
			total = total + sample * sample
		end
		memo[a] = total
		return total
	end
	local function sortTest(a, b)   return sortValue(a) < sortValue(b) end
	local function unSortTest(a, b) return sortValue(a) > sortValue(b) end
	
	table.sort(seg, self.reverse and unSortTest or sortTest)
	
	if self.collate then
		local stacks = {} for i=1,self.collate do table.insert(stacks,{}) end
		for i,v in ipairs(seg) do table.insert(stacks[math.random(#stacks)], v) end
		seg = {}
		for i1,v1 in ipairs(stacks) do for i2,v2 in ipairs(v1) do
			table.insert(seg, v2)
		end end
	end
	
	if self.normalize then
		for i,v in ipairs(seg) do
			local max = 0
			for s=0,v.len-1 do
				local sample = math.abs(v.data[s])
				if sample > max then max = sample end
			end
			
			for s=0,v.len-1 do
				v.data[s] = v.data[s] / max * self.normalize
			end
		end
	end
	
	self.seg = engine.ffi.new(string.format("struct sample*[%d]", #seg), seg)
	self.seglen = #seg
end