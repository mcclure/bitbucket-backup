addScreen("cubefront", km.roomObj, true)

au.rainbow:setVolume(0)

local bots = {}
for i=1,11 do
	table.insert(bots, string.format("bot%d",i))
end

if fm.robotdead then
	for i,v in ipairs(bots) do
		bridge:setVisible(id(v),false)
	end
end

plantScreen(screens["cubefront"])

if fm.robotdead and not fm.helmet then
	addScreen("cube_helmet", km.roomObj)
end

function onHelmet()
	fm.helmet = true
	trashScreen("cube_helmet")
	
	act:push({"do", this=function()
		shaders.cubefront.radius:set(km.helmetfactor/surface_width/2)
	end})
end

gm.robot_discussion = Dialogue({
	gstart = "basic",	
	blank=bots,
	create="cube_robot",
	tree = {
		start = {
			say={"Diamond-encrusted dog shit.","A million points of lifeless light. All sterile. All just filling space.","What is it you're doing here."},
			options={ {say="Who are you?",go="resp1"}, {say="I'm kind of starting to freak out.",go="resp2"} }},
		resp1 = {
			say={"No, don't even start with that."},
			options={ {say="Why are you so grumpy?",go="resp11"}, {say="Can you help me?",go="resp12"} }},
		resp2 = {say={"Yeah. You get used to it."}},
		resp11 = {
			say={"Look. You ever ride a motorcycle? Like, really moving. You ever feel what it waslike to live?"},
			options={ {say="What?",go="resp11x"} }},
		resp11x = {
			say={"A motorcycle. Ever ride one?"},
			options={ {say="What's that have to do with anything?",go="resp111"}, {say="I have a motorcycle.",go="resp112"} }},
		resp12 = {say={"Not the help you really need."}},
		resp111 = {say={"Never mind."}},
		resp112 = {say={"Oh. You need a helmet?"}, options={ {say="Yes.",go="resp1111"}, {say="No.",go="resp111"} }},
		resp1111 = {
			say={"I guess I'm not using this anymore\nanyway."},
			onDismiss=function(opt, di)
				fm.robotdead = true
				gm.robot_discussion.blank = {}
				di:teardown()
				addScreen("cube_helmet", km.roomObj)
			end,
		},
	}
})