-- Game logic update

playerPos = getPosition(player)

-- Keypresses -- movement

local move = nil
function mm() if not move then move = a(Vector3(0,0,0)) end end

if not (cm.drop or levelmsg) then -- Check whether "normal" movement is happening
	-- Did we move?
	if down[KEY_w] or down[KEY_s] then
		mm() move.z = down[KEY_w] and -km.strafe or km.strafe
	end
	if down[KEY_a] or down[KEY_d] then
		mm() move.x = down[KEY_d] and km.strafe or -km.strafe
	end
	
	-- Did we win the level?
	local exitPos = getPosition(exit)
	local dist = vNorm(vSub(playerPos,exitPos))
	if dist < km.winunder then
		move = nil
		stopWorld()
		newAlert("You reach the exit safely.")
	end
end

if move then -- Enforce "normal" movement
	if not cm.moving then
		local ppe = s:getPhysicsEntityBySceneEntity(player)
		cm.friction = bridge:getFriction(ppe)
		cm.moving = true
		bridge:setFriction(ppe, 0)
	end
	s:setVelocity(player, a(bridge:rotateXzByAngle(move, -cm.angle)))
	bridge:activate(s:getPhysicsEntityBySceneEntity(player))
else
	if cm.moving then
		local ppe = s:getPhysicsEntityBySceneEntity(player)
		bridge:setFriction(ppe, cm.friction)
	end
end

-- Debug keys

local mouse_mod_change
if pressed[KEY_0] then km.fps_counter = not km.fps_counter end
if pressed[KEY_MINUS] then mouse_mod_change = 1/1.1 end
if pressed[KEY_EQUALS] then mouse_mod_change = 1.1 end
if mouse_mod_change then
	mouse_mod = mouse_mod * mouse_mod_change
	local formatter = mouse_mod < 0.1 and "%0.3f" or "%0.2f"
	newAlert(string.format("Mouse sensitivity: "..formatter, mouse_mod))
end

if _DEBUG then
	if pressed[KEY_i] then
		cm.mouseLooking = not cm.mouseLooking
		player:setColor(1,1,1,(cm.mouseLooking and 0 or 1))
	end
	if pressed[KEY_o] then km.on_player_hit = 0 newAlert("(Invincible)") end
	if pressed[KEY_p] then newAlert("FPS " .. Services.Core:getFPS() .. " ENEMIES " .. enemycount) end
	if pressed[KEY_l] then newAlert(to_string(a(bridge:rotateXzByAngle(a(Vector3(0,0,-km.strafe)), -cm.angle)))) end
	if pressed[KEY_k] then Services.Core:enableMouse(true) end
end

-- Handle mouse

if cm.mouseLooking and mouseAt then -- Actual mouse rotation
	if cm.mouseLookFrom then
		local dist = vSub(mouseAt,cm.mouseLookFrom)
		if dist.x ~= 0 then
			cm.angle = cm.angle + km.turn*dist.x*mouse_mod
		end
--		if dist.y ~= 0 then rotLike(0,-dist.y) end
		vSet(cm.mouseLookFrom, mouseAt)
	else
		cm.mouseLookFrom = vDup(mouseAt)
	end
elseif cm.mouseLookFrom then
	delete(cm.mouseLookFrom)
	cm.mouseLookFrom = nil
end

if mouseAt and (mouseAt.y > 3*surface_height/4 or mouseAt.y < surface_height/4 -- Mouse wraparound
	or mouseAt.x > 3*surface_width/4 or mouseAt.x < surface_width / 4) then
	cdelete(cm.mouseLookFrom)
	cm.mouseLookFrom = nil
	bridge:warpCursor(surface_width/2,surface_height/2)
end

if not cm.drop and pressed[0] or pressed[KEY_SPACE] then -- Mouse button
	playerShot(playerPos, -cm.angle, true)
end

-- Run machines

if not cm.drop then enemyUpdate() end
runShots()
runSpecial()
updateCamera()
control_moving:updateMusic()
control_panic:updateMusic()

-- DOS management

if levelmsg and playerPos.y < km.dismisslevelmsgat then -- Should we set up new message?
	s_land:Play()
	dosclear(11)
	levelmsg = false
	newSignal( signalMsg() )
end

if topscroll_msg then -- Manage top message
	dosclear(0)
	local terminate_at = #topscroll_msg + (topscroll_blackout and 2 or 40)
	local across = math.floor((ticks - topscroll_start) / km.scroll_topat)
	if across < terminate_at then
		if not topscroll_blackout or across < terminate_at-2 or stickyrandom() then
			dos:set(40-across,0,topscroll_msg)
		end
	else
		topscroll_msg = nil
	end
end

if bottomscroll_queue.count > 0 or need_alert_cleanup then -- Manage bottom messages
	for i = 1,km.max_bottomlines do
		dosclear(24-i)
	end
	for _i = 1,bottomscroll_queue.count do
		local i = _i--bottomscroll_queue.count + 1 - _i
		local msg = bottomscroll_queue[bottomscroll_queue.low + bottomscroll_queue.count - i]
		local y = 24 - _i
		dos:set(0,y,msg)
	end
	if ticks - bottomscroll_start >= km.scroll_bottomat then
		bottomscroll_queue:pop()
		bottomscroll_start = ticks
		need_alert_cleanup = true
	else
		need_alert_cleanup = false
	end
end

if km.fps_counter then dos:set(0,24-5, Services.Core:getFPS().."~FPS") end
if km.health_bar then dos:set(40-9,24-5,"Health:~" .. gm.player_hp) end

-- If we need to shut down this automata completely, do that last.

if cm.win_now then
	if gm.killed_by then
		bridge:load_room_txt("media/dead.txt")
	elseif gm.won_whole_game then
		if gm.at_level == 1 then
			bridge:load_room_txt("media/ending.txt")
		else
			gm.at_level = gm.at_level - 1
			bridge:rebirth()
		end
	else
		gm.at_level = gm.at_level + 1
		bridge:rebirth()
	end
end

pressed = {}