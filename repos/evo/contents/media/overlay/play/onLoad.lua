-- Gameplay, On Load

pull(gm, {p=PhysicsScreen(screen()), canjump = false, what={}, exits={}, lava={}, enemies={}, s_land_last = -1000})
pull(km, {jump_refract = 2, introflick=30, introfor=5, landevery=4})

function reset_session()
	session = {
		deaths=0, totalf = 0, lastf = 0, won=0,
		f={}
	}
end
if not session then
	reset_session()
end

function levelend(success, touched, special_message)
	gm.player:setColor(0,0,0,0)
	gm.dead = true
	if gm.playerat then
		session.real = true
		session.won = success and 1 or 0
		if not gm.won then session.deaths = session.deaths + 1 end
		local lap = ticks - gm.playerat
		session.lastf = lap
		session.totalf = session.totalf + lap
		table.insert(session.f, lap)
	end
	local top = a(gm.player:getPosition()).y < gm.level.ydim*gm.level.scale/2
	local safeline = top and 21 or 0
	if success then
		gm.won = true
		s_win:Play(false)
		fill(dos, 0,safeline,40,3,"#")
		dos:set_centered(0,safeline+1,40, special_message or "YOU#ARE#CONGRATULATIONS!!!", false)
		dos:set_centered(0,safeline+2,40, "PRESS#\010#TO#CONTINUE", false)
	else
		gm.deadat = ticks
		gm.fatal = touched
		if special_message then
			s_stuck:Play(false)
		else
			s_lose:Play(false)
		end
		fill(dos, 0,safeline,40,3,"#")
		dos:set_centered(0,safeline,40,   special_message or "DEAD!!!!#YOU#ARE#A#TERRIBLE#PLAYER", false)
		dos:set_centered(0,safeline+1,40, "PRESS#\010#TO#CONTINUE", false)
		dos:set_centered(0,safeline+2,40, "(OR#Q#IF#THIS#LEVEL#IS#IMPOSSIBLE)", false)
		force_load = gm.level
	end
end

function startWithSpec(level)
	gm.level = level
	gm.level:load(gm)
	bridge:register_room_onCollision(gm.player, function()
		local _b = bridge:room_b()
		local b = _b.__ptr and gm.what[_b.__ptr]
		if not gm.dead then
			if b and not b.e and b.c == 2 then
				levelend(false, _b)
			elseif b and b.e and b.c == 2 then
				levelend(true, _b)
			elseif (not gm.lastjump) or ticks-gm.lastjump > km.jump_refract then
				if not gm.canjump and (ticks-gm.s_land_last)>km.landevery then
					gm.s_land_last = ticks
					s_land:Play(false)
				end
				gm.canjump = true;
			end
		end
	end)
end

-- SOUND

if not have_audio then
	have_audio = true
	
	-- Generate jump sound
	if s_jump then delete(s_jump) end
	do
		local data = a(NumberArray())
		local limit = (512*4)-1
		local v
		for i=0,limit do
			v = ((math.floor(i/40)%2)==0) and 0.25 or -0.25
			data:push_back(v*((limit-i)/limit))
		end
		s_jump = bridge:soundFromValues(data)
	end
	
	if s_land then delete(s_land) end
	do
		local data = a(NumberArray())
		local limit = (512*4)-1
		local v
		for i=0,limit do
			v = ((math.floor(i/320)%2)==0) and 0.25 or -0.25
			data:push_back(v*((limit-i)/limit))
		end
		s_land = bridge:soundFromValues(data)
	end
	
	if s_win then delete(s_win) end
	do
		local data = a(NumberArray())
		local limit = (512*32)-1
		local v
			local w = 320
		for i=0,limit do
			v = ((math.floor(i/w)%2)==0) and 0.25 or -0.25
			if i % math.floor(limit/3) == 0 then w = w / 2 end
			data:push_back(v)--v*((limit-i)/limit))
		end
		s_win = bridge:soundFromValues(data)
	end
	
	if s_lose then delete(s_lose) end
	do
		local data = a(NumberArray())
		local limit = (512*32)-1
		local v
			local w = 40
		for i=0,limit do
			v = ((math.floor(i/w)%2)==0) and 0.25 or -0.25
			if i % math.floor(limit/3) == 0 then w = w * 2 end
			data:push_back(v)--v*((limit-i)/limit))
		end
		s_lose = bridge:soundFromValues(data)
	end
	
	if s_stuck then delete(s_stuck) end
	do
		local data = a(NumberArray())
		local limit = (512*32)-1
		local v
			local w = 320
		for i=0,limit do
			v = ((math.floor(i/w)%2)==0) and 0.25 or -0.25
			--if i % math.floor(limit/3) == 0 then w = w * 2 end
			data:push_back(v)--v*((limit-i)/limit))
		end
		s_stuck = bridge:soundFromValues(data)
	end
end

-- "The actual game" 

makeDos()
if force_load then
	local will_force_load = force_load
	force_load = nil -- TODO -- not in debug mode
	startWithSpec(will_force_load)
else
	local put = nil
	if session and session.real then
		local keep_session = session
		reset_session()
		put = {version=1.0, pid=player_info.id, deaths=keep_session.deaths, totalf=keep_session.totalf, won=keep_session.won, gid=keep_session.gid, sid=keep_session.sid, lastf=keep_session.lastf,}
		if keep_session.deaths then
			local rms = 0
			for i,v in ipairs(keep_session.f) do
				rms = rms + v*v
			end
			rms = math.sqrt(rms / keep_session.deaths)
			put.rms = rms
		end
	end
	dosLoading()
	xhr(um.level, function(t)
		dos:clear()
		pull(session, {gid=t.gid, sid=t.sid})
		startWithSpec(Spec(t))
	end, put)
end

-- Set up input handler
down = {} pressed = {}
class "Input" (EventHandler)
function Input:Input()
	self:EventHandler()
end

function Input:handleEvent(e)
	if e:getDispatcher() == Services.Core:getInput() then
		local inputEvent = InputEvent(e)
		local key = inputEvent:keyCode()
		if e:getEventCode() == EVENT_KEYDOWN then
			down[key] = true
			pressed[key] = true
		elseif e:getEventCode() == EVENT_KEYUP then
			down[key] = false
		end
	end
end

do
	input = Input()
	
	Services.Core:getInput():addEventListener(input, EVENT_KEYDOWN)
	Services.Core:getInput():addEventListener(input, EVENT_KEYUP)
end