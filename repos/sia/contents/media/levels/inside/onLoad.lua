-- "Inside"

-- Audio

if not au.alone then
	au.alone = Sound("media/ArtieShaw-AloneTogether1939.ogg")
end
if not aq and not au.alone:isPlaying() then
	au.alone:Play(true)
	fm.alonep = 1.0
	au.alone:setPitch(fm.alonep)
	au.alone:setOffset(44100*7+24000)
end
au.alone:setVolume(1.0)

-- Actually display room

addScreen("inside", km.roomObj, true)

local helmet = {}
for i=1,3 do
	table.insert(helmet, string.format("helmet%d",i))
end

if not fm.tapedhelmet then
	for i,v in ipairs(helmet) do
		bridge:setVisible(id(v),false)
	end
end

plantScreen(screens["inside"])

-- Blank out red circles

act:push({"do", this=function()
	for i=1,35 do
		local oops = id(string.format("oops%d",i))
		bridge:room_remove_screen(oops, false)
		screens["inside"]:removeChild(oops)
		delete(oops)
	end
end})

-- Click handlers

function door()
	if gm.dialog then return end
	if gm.open then
		go("airlock")
	else
		for i,v in ipairs({"cover4","cover3","cover2","cover1"}) do
			act:push({"disable", target=id(v), wait=km.wait})
		end
		gm.open = true
	end
end

function backdoor()
	if gm.dialog then return end
	local backdoor_note = Dialogue({
		gstart = "basic",
		tree = { start = { say={"The door refuses to budge."}, } }
	})

	backdoor_note:invoke()
end

function tapeclick()
	if gm.dialog then return end
	local taper_note = nil
	
	if not fm.helmet then 
		taper_note =
			Dialogue({
				gstart = "basic",
				tree = { 
					start = { 
						say={"Oh hi!!! I'm here!!! I'm moving!!! I canmove my legs!!!"}, 
						options= {{say="What?",go="resp1"}, {say="The silver moon. Does it yet live?",go="resp2"},}
					},
					resp1 = { 
						say={"Yes!!! It's wonderful!!! I'm so happy\nyou're here!!!"}, 
						options= {{say="None of this means anything to me.",go="resp11"}, {say="Are you... made of tape reels, or...?",go="resp12"},}
					},
					resp11 = { 
						say={"What's important is that we're together now."},
					},
					resp12 = { 
						say={"Life is full of mystery!!!"},
					},
					resp2 = { 
						say={"Things are much the same as ere last we spoke. The Unwatcher grows ever more\nagitated-- though whether more\nvulnerable, I cannot say."}, 
						options= {{say="Go on.",go="resp21"},}
					},
					resp21 = { 
						say={"Oh, no doubt have I you wish to hear\nmore? But little more is there to say.\nFear not. Alignments are drifting into\nphase; soon will be time to make your\nmove."},
					},
				},
			})
	else
		taper_note =
			Dialogue({
				gstart = "basic",
				tree = { 
					start = { 
						say={"Oh wow!!! That is such a cool helmet!!! I wish I had a helmet like that!!!","Can I have your helmet???"}, 
						options= {{say="Yes.",go="yes"}, {say="No.",go="no"},}
					},
					no = { 
						say={"I respect your decision!!!"},
					},
					yes = {
						say={"You're the best!!!"},
						onDismiss=function(opt, di)
							fm.helmet = false
							fm.tapedhelmet = true
							di:teardown()
							act:push({"do", this=function()
								shaders.inside.radius:set(km.nohelmetfactor/surface_width/2)
							end})
							for i,v in ipairs(helmet) do
								act:push({"enable", target=id(v), wait=km.wait})
							end
						end,
					},
				},
			})

	end

	taper_note:invoke()

end

if _DEBUG then
function Q(t) -- Shut! Up!
au.alone:Stop()
aq = t
end
end