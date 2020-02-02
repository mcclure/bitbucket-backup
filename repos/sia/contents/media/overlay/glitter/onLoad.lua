-- Glitter machine -- load

gltm = {
}

gltmap = {
	basic ={ {2/7,2/128,0},  t = {{0.01, "basic2"}, {0.001, "basic3"}, {0.001, "basic4"}, {0.01, "baout1"}, }, },
	basic2={ {1/7,2/128,0},  t = {{0.5,  "basic"}, }, },
	basic3={ {2/7,3/128,0},  t = {{0.05,  "basic"}, }, },
	basic4={ {1/7,1/128,0},  t = {{0.005, "basic"}, }, },
	baout1={ {1,0,0},        t = {{1.0,  "baout2"}, }, },
	baout2={ {1,4/128,8/128},t = {{0.05,  "basic"},  }, },
	
	cohere={ {2/7,2/128,0}, t={{0.025, "coher2"}},},
	coher2={ {4/7,2/128,0}, t={{0.1, "coher3"}},},
	coher3={ {1/2,0,0},     t={{0.1, "coher4"}},},
	coher4={ {1,0,0}, },
}

function glitter_update(_g,_a)
	if _g then gltm.glittering = _g end
	if _a then gltm.glitterat = _a end

	local ing = gltm.glittering
	local atn = gltm.glitterat

	if ing and atn then
		local at = gltmap[atn]
		if gltm.wasat ~= atn then
			dos:glitch(unpack(at[1]))
			gltm.wasat = atn
		end
		local t = at.t
		if not dontmark and t then
			local sofar = 0
			local target = math.random()
			for i,v in ipairs(t) do
				sofar = sofar + v[1]
				if sofar > target then
					gltm.glitterat = v[2]
					break
				end
			end
		end
	end
end