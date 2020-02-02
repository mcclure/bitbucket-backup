-- Editor on load

if not egm then egm = {} end
pull(km, {cursor_flicker_time=5, cursor_flicker_count = 2, cursor_highlight_count=20})

local color_messages = {"Enter red (0-8):","Enter green (0-8):","Enter blue (0-8):"}

class "CursorController" (Controller)
function CursorController:CursorController(_spec)
	local spec = tableCopy(_spec)
	spec.pressed = tableMerge({
		cursor_sw=self.move_sw, cursor_se=self.move_se, cursor_nw=self.move_nw, cursor_ne=self.move_ne,
		cursor_hello=self.flicker, cursor_refloor=self.refloor, cursor_defloor=self.defloor, cursor_defloor_entity=self.defloor,
		cursor_specialfloor=self.specialfloor, cursor_dragup=self.drag, cursor_dragdown=self.drag, cursor_entity=self.entity,
		cursor_copy=self.copy, cursor_copy_ephermal=self.copy, cursor_copy_entity=self.copy, cursor_copy_floor=self.copy_floor, cursor_paste=self.paste, 
		cursor_room=self.room, cursor_edit=self.edit_menu, cursor_whatis=self.whatis, editor_autosave=self.autosave,
		cursor_temp_recenter=self.temp_recenter, cursor_temp_zoomin = self.temp_zoom, cursor_temp_zoomout = self.temp_zoom,
		editor_save=self.editorsave, editor_load=self.editorload}, spec.pressed)
	spec.make = tableMerge({special="transient"}, spec.make)
	Controller.Controller(self,spec)
	self.level:add(self, spec.x, spec.y, self.level.theme:makeCursor(self))
	self.e.visible = false
end

function CursorController:move(dir)
	Controller.move(self,dir)
	self:flicker()
end

local triggerCursorColor = {0,1,1}
local anchorCursorColor = {1,0,1}

function CursorController:flicker(also)
	if not also then
		self.e.visible = true
		self.flickerAt = ticks
	else
		local flashat = Queue()
	
		for k,v in self.level.map:iter() do
			for k2,v2 in pairs(v) do
				if v2.kind == km.TRIGGER then
					flashat:push({x=k.x, y=k.y, color=triggerCursorColor})
				end
				if v2.kind == km.ANCHOR then
					flashat:push({x=k.x, y=k.y, color=anchorCursorColor})
				end
			end
		end
	
		Ent({parent=self, flashat=flashat, theme=self.level.theme, onTick = function (self)
			if self.e and (ticks-self.since>km.cursor_highlight_count) then killScreen(self.e) self.e = nil end
			if not self.e then
				self.since = ticks
				local n = self.flashat:pop()
				if n then
					self.e = self.theme:makeCursor(n)
				else
					self.parent.e.visible = true
					self.parent.flickerAt = ticks
					self:die()
				end
			end
		end}):insert()
	end
end

function CursorController:drag(which)
	local dir = which == "cursor_dragup" and 1 or -1
	local current = gm.level:find(self.x,self.y,km.BLOCK)
	local make = self:floor_remake(current)
	make.height = math.max( (make.height or 0) + dir, 0 )
	loadent(make) self:fixup_ents(self.x,self.y)
end

function CursorController:copy(which)
	local typ = nil
	if which == "cursor_copy_block" then typ = km.BLOCK
	elseif which == "cursor_copy_entity" then typ = km.SPRITE
	elseif which == "cursor_copy_ephermal" then typ = km.TRIGGER end

	if typ then
		local make = gm.level:find(self.x,self.y,typ)
		make = make and make.make
		if make then
			egm.clipboard = tableDeepCopy(make)
		end
	else
		NumberEater({delegate=self, delegateFunction=self.copy_menu, message="Copy what?",
			topmessage = {"(1) Floor","(2) Entity","(3) Trigger"}
		}):insert()
	end
end

function CursorController:copy_menu(i)
	if i == 1 then
		self:copy("cursor_copy_block")
	elseif i == 2 then
		self:copy("cursor_copy_entity")
	elseif i == 3 then
		self:copy("cursor_copy_ephermal")
	end
end

function CursorController:paste()
	if not egm.clipboard then return end
	local current = gm.level:find(self.x,self.y,egm.clipboard.kind)
	self:floor_remake(current)
	loadent(tableMerge(egm.clipboard, {x=self.x, y=self.y}))
	self:fixup_ents(self.x,self.y)
end

function CursorController:onTick()
	if self.flickerAt then
		local hiddenance = math.floor((ticks - self.flickerAt)/km.cursor_flicker_time)
		if hiddenance > km.cursor_flicker_count*2 then
			self.e.visible = false
			self.flickerAt = nil
		else
			self.e.visible = 0 == hiddenance%2
		end
	end
end

function CursorController:whatis()
	self.e.visible = false
	self.flickerAt = nil
	bridge:term_setHidden(false)
	local output = "Press tab to escape this nether world\n"
	local tile = gm.level.map:get(self.x,self.y)
	
	if tile then
		for i,v in pairs(tile) do
			local kind = "UNKNOWN (this should never happen)"
			if v.kind == km.BLOCK then kind = "FLOOR" end
			if v.kind == km.SPRITE then kind = "SPRITE" end
			if v.kind == km.TRIGGER then kind = "TRIGGER" end
			if v.kind == km.ANCHOR then kind = "ANCHOR" end
			output = output .. "\n" .. kind .. ":"
			for k2,v2 in pairs(v.make) do
				if k2 ~= "kind" then
					output = string.format("%s %s:%s",output, k2,to_string(v2))
				end
			end
		end
	else
		output = output .. "\nThere is nothing here."
	end
	print(output)
end

function CursorController:temp_recenter()
	gm.level.cm.x,gm.level.cm.z = self.x,self.y
	gm.level:updateCamera()
end

function CursorController:temp_zoom(which)
	print(which)
	local dir = which == "cursor_temp_zoomout" and 2 or 0.5
	local current = gm.level.cm.zoom or 1
	gm.level.cm.zoom = current * dir
	gm.level:updateCamera()
end

-- MENU menu

function CursorController:edit_menu()
	NumberEater({delegate=self, delegateFunction=self.edit_menu_result, message="Select menu:",
		topmessage = {"(1) Edit floor here","(2) Edit visible entity here","(3) Edit invisible trigger here","(4) Edit room",}
	}):insert()
end

function CursorController:edit_menu_result(i)
	if i == 1 then
		self:refloor()
	elseif i == 2 then
		self:entity()
	elseif i == 3 then
		self:specialfloor()
	elseif i == 4 then
		self:room()
	end
end

-- ENTITY menu

function CursorController:entity()
	local current = gm.level:find(self.x,self.y,km.SPRITE)
	if not current then return self:entity_place() end
	
	NumberEater({delegate=self, delegateFunction=self.entity_menu, message="Select option:",
		topmessage = {"(1) Replace entity.","(2) Set entity warp-on-push.","(3) Set entity trigger-on-push.","(4) Set entity flip.","(5) Set entity color.","(6) Delete entity.",}
	}):insert()
end

function CursorController:entity_menu(i)
	if i == 1 or i == 6 then
		local current = gm.level:find(self.x,self.y,km.SPRITE)
		self:floor_remake(current)
	end
	if i == 1 then
		self:entity_place()
	elseif i == 2 then
		NumStack({delegate=self, delegateFunction=self.entity_warpto, messages={"Enter target filename: ","Enter spawn number: "}, spawners={StringEater,NumberEater}}):run()
	elseif i == 3 then
		StringEater({delegate=self, delegateFunction=self.entity_trigger, message="Enter trigger name: "}):insert()
	elseif i == 4 then
		NumberEater({delegate=self, delegateFunction=self.entity_flip, message="Should face: ",
			topmessage = {"(1) Left","(2) Right"}
		}):insert()
	elseif i == 5 then
		NumStack({delegate=self, delegateFunction=self.entity_color, messages=color_messages}):run()
	end
end

function CursorController:entity_place()
	StringEater({delegate=self, delegateFunction=self.entity_result, message="Enter entity filename: "}):insert()
	local current = gm.level:find(self.x,self.y,km.SPRITE)
	self:floor_remake(current)
end

function CursorController:entity_trigger(str)
	local current = gm.level:find(self.x,self.y,km.SPRITE)
	local make = self:floor_remake(current)
	make.act = str make.warpto = nil make.warpanchor = nil
	loadent(make)
end

function CursorController:entity_warpto(t)
	local current = gm.level:find(self.x,self.y,km.SPRITE)
	local make = self:floor_remake(current)
	make.act = nil make.warpto = t[1] make.warpanchor = t[2]
	loadent(make)
end

function CursorController:entity_flip(t)
	local current = gm.level:find(self.x,self.y,km.SPRITE)
	local make = self:floor_remake(current)
	make.flip = t == 2
	loadent(make)
end

function CursorController:entity_color(t)
	local current = gm.level:find(self.x,self.y,km.SPRITE)
	local make = self:floor_remake(current)
	make.color = t
	loadent(make)
end

function CursorController:entity_result(t)
	gm.level:createSprite({x=self.x,y=self.y,sprite=t,flip=true})
end

-- FLOOR ATTRIB MENU

class "NumStack"
function NumStack:NumStack(spec)
	pull(self,{at=1, result={}})
	pull(self,spec)
end
function NumStack:run()
	if self.at > #self.messages then
		local delegateFunction = self.delegateFunction or self.delegate.onStack
		if self.delegate then delegateFunction(self.delegate, self.result) end
		return
	end
	local SpawnClass = self.spawners and self.spawners[self.at] or NumberEater
	SpawnClass({delegate=self, delegateFunction=self.next, message=self.messages[self.at]}):insert()
end
function NumStack:next(i)
	table.insert(self.result, i)
	self.at = self.at + 1
	self:run()
end

-- Note: MenuEater cannot have a delegate.
class "MenuEater" (NumberEater)

function MenuEater:MenuEater(spec)
	self.message = spec.message or "Select option:"
	Eater.Eater(self, spec)
	if not self.delegate then self.delegate = self end
	if not self.topmessage then
		self.topmessage = {}
		for i,item in ipairs(self.menu) do
			table.insert(self.topmessage, string.format("(%d) %s", i, item.message))
		end
	end
end

function MenuEater:onNumber(i)
	if not self.menu then return end
	local item = self.menu[i]
	if not item then return end
	
	item.act(self)
end

-- Does have delegates and delegatefunctions, but performs them after completing its dark work
class "AutoEater" -- NOT ACTUALLY AN EATER!

function AutoEater:AutoEater(spec)
	pull(self, spec)
	self.child = self.spawn(tableMerge({delegate=self, delegateFunction=self.consume},spec.spec))
	self.spawn = nil self.spec = nil
end

function AutoEater:insert()
	self.child:insert()
end

function AutoEater:consume(i)
	-- TODO: multiset targets/keys?
	if self.filter then i = self.filter(i) end
	if self.target and self.key then self.target[self.key] = i end

	local delegateFunction = self.delegateFunction or self.delegate.onConsume
	if self.delegate then delegateFunction(self.delegate, i, self) end
end

function CursorController:refloor()
	NumberEater({delegate=self, delegateFunction=self.refloor_menu, message="Select option:",
		topmessage = {"(1) Set floor height.","(2) Set floor color.","(3) Texture","(4) Texture mode", "(4) Place light.",}
	}):insert()
end

function CursorController:refloor_menu(i)
	if i == 1 then
		NumberEater({delegate=self, delegateFunction=self.refloor_height, message="Enter a height (0-9):"}):insert()
	elseif i == 2 then
		NumStack({delegate=self, delegateFunction=self.refloor_color, messages=color_messages}):run()
	elseif i == 3 then
		StringEater({delegate=self, delegateFunction=self.refloor_texture, message="Enter texture name: "}):insert()
	elseif i == 4 then
		NumberEater({delegate=self, delegateFunction=self.refloor_textype, message="Select texture mode:",
			topmessage = {"(1) Glitch","(2) Tile","(3) Stretch",}
		}):insert()
	elseif i == 5 then
		NumStack({delegate=self, delegateFunction=self.refloor_light, messages={"Enter light radius (1-9 or 0 for point):","Enter light intensity (0-9):"}}):run()
	end
end

function destroyEnt(x,y,obj)
	gm.level.map:get(x,y)[obj.id] = nil
	obj:die()
end

function CursorController:floor_remake(current)
	local make
	if current then
		make = current.make
		destroyEnt(self.x,self.y,current)
	else
		make = {x=self.x, y=self.y, kind=km.BLOCK}
	end
	return make	
end

function CursorController:fixup_ents(x,y)
	local current = gm.level:find(x,y,km.SPRITE)
	if current then gm.level:spritePos({x=x,y=y,e=current.e}) end
end

function CursorController:refloor_height(i)
	local current = gm.level:find(self.x,self.y,km.BLOCK)
	local make = self:floor_remake(current)
	make.height = i
	loadent(make) self:fixup_ents(self.x,self.y)
end

function CursorController:refloor_textype(i)
	local current = gm.level:find(self.x,self.y,km.BLOCK)
	local make = self:floor_remake(current)
	make.textype = i
	loadent(make)
end

function CursorController:refloor_color(t)
	local block = self.level:find(self.x,self.y,km.BLOCK)
	local c = {t[1]/8,t[2]/8,t[3]/8}
	if _DEBUG then block.make.color = c end
	block.e:setColor(c[1],c[2],c[3],1)
end

function CursorController:refloor_texture(t)
	local current = gm.level:find(self.x,self.y,km.BLOCK)
	local make = self:floor_remake(current)
	make.texture = t
	loadent(make)
end

function CursorController:refloor_light(t)
end

function CursorController:defloor(which)
	gm.level:clear(self.x,self.y)
end

-- TRIGGER MENU

function CursorController:specialfloor()
	NumberEater({delegate=self, delegateFunction=self.specialfloor_menu, message="Select option:",
		topmessage = {"(1) Place link","(2) Place player spawn","(3) Place trigger","(4) Move-camera trigger","(5) Delete all trigger/special this space"}
	}):insert()
end

function CursorController:specialfloor_menu(i)
	if i == 1 then
		NumStack({delegate=self, delegateFunction=self.linkfloor_result, messages={"Enter target filename: ","Enter spawn number: "}, spawners={StringEater,NumberEater}}):run()
	elseif i == 2 then
		NumStack({delegate=self, delegateFunction=self.specialfloor_anchor, messages={"Enter a spawn # (0-9): ","Recenter camera on landing? (0=no 1=yes 2=special 3=dontcare)"}}):run()
	elseif i == 3 then
		StringEater({delegate=self, delegateFunction=self.specialfloor_trigger, message="Enter trigger name: "}):insert()
	elseif i == 4 then
		self:specialfloor_trigger("recenter_camera")
	elseif i == 5 then
		while true do
			local current = gm.level:find(self.x,self.y,km.TRIGGER)
			if not current then break end
			self:floor_remake(current)
		end
		while true do
			local current = gm.level:find(self.x,self.y,km.ANCHOR)
			if not current then break end
			self:floor_remake(current)
		end
	end
end

function CursorController:specialfloor_trigger(str)
	local current = gm.level:find(self.x,self.y,km.TRIGGER)
	self:floor_remake(current)
	gm.level:createSpecial({x=self.x,y=self.y,act=str})
end

function CursorController:linkfloor_result(t)
	local current = gm.level:find(self.x,self.y,km.TRIGGER)
	self:floor_remake(current)
	gm.level:createSpecial({x=self.x,y=self.y,warpto=t[1],warpanchor=t[2]})
end

function CursorController:specialfloor_anchor(t)
	local current = gm.level:find(self.x,self.y,km.ANCHOR)
	self:floor_remake(current)
	local make = {x=self.x,y=self.y,anchor=t[1],kind=km.ANCHOR}
	if t[2] == 0 or t[2] == 1 then -- notice subtle thing: otherwise recenter will be nil, which is treated different from false
		make.recenter=(t[2]==1)
	end
	
	gm.level:createSpecial(make)
	
	if t[2] == 2 then
		NumberEater({delegate=self, delegateFunction=self.specialfloor_anchor_2, message="Select camera anchor:",}):insert()
	end
end

function CursorController:specialfloor_anchor_2(i)
	local current = gm.level:find(self.x,self.y,km.ANCHOR)
	local make = self:floor_remake(current)
	make.recenter = true
	make.recenter_anchor = i
	gm.level:createSpecial(make)
end

-- ROOM MENU

function CursorController:room()
	local menu={
		{message="Set background color",
		 act=function() NumStack({delegate=self, delegateFunction=self.room_color, messages=color_messages}):run() end},
	--	{message="Place player (at cursor)",
	--	 act=function() NumStack({delegate=self, delegateFunction=self.room_color, messages=color_messages}):run() end},
		{message="Center camera (at cursor)",
		 act=function()
			gm.level.meta.camat = {x=self.x,y=self.y}
			gm.level:reset()
		 end},
		{message="Set zoom",
		 act=function()
			StringEater({delegate=self, delegateFunction=self.zoom_result, message="Enter zoom factor: ", seed=string.format("%0.3f",gm.level.meta.camzoom or 1)}):insert()
		 end},
		{message="Clear camera/zoom",
		 act=function()
			gm.level.meta.camat = nil
			gm.level:reset()
		 end},
		{message="Set music",
		 act=function()
			StringEater({delegate=self, delegateFunction=self.sound_result, message="Enter music file: ",}):insert()
		 end},
		{message="Set fixed camera",
		 act=function()
			NumberEater({delegate=self, delegateFunction=self.room_fixedcamera, message="Fixed camera: ",
				topmessage = {"(1) On","(2) Off",}
			}):insert()
		 end},
		{message="Edit background",
		 act=function()
			local bgmenu = {
				{message="Set background image",
				 act=function()
					AutoEater({spawn=StringEater, spec={message="Background image name:"}, target=gm.level.meta, key="bgimage",
						delegate=gm.level, delegateFunction=gm.level.reset}):insert()
				 end},
				{message="Remove background image",
				 act=function()
					gm.level.meta.bgimage = nil
					gm.level:reset()
				 end},
				{message="Set background zoom",
				 act=function()
					AutoEater({spawn=StringEater, spec={message="Enter a number (example: 1.0):"}, target=gm.level.meta, key="bgzoom",
						delegate=gm.level, delegateFunction=gm.level.reset, filter=tonumber}):insert()
				 end},
				{message="Set background stretch/tile",
				 act=function()
					AutoEater({spawn=NumberEater, spec={message="What do you want the background to do:",
						topmessage = {"(1) Stretch to fill top-bottom","(2) Stretch to fill left-right","(3) Stretch to fill screen exactly","(4) Tile 1:1", "(5) Tile & stretch to fill top-bottom"}},
						target=gm.level.meta, key="bgmode", delegate=gm.level, delegateFunction=gm.level.reset}):insert()
				 end},
			}
			MenuEater({menu=bgmenu}):insert()
		 end},
	}

	MenuEater({menu=menu}):insert()
end

function CursorController:room_fixedcamera(i)
	gm.level.meta.camfixed = (i == 1)
	gm.level:reset()
end

function CursorController:zoom_result(s)
	local c = tonumber(s)
	if not c or c <= 0 then return end
	gm.level.meta.camzoom = c
	gm.level:reset()
end

function CursorController:sound_result(s)
	gm.level.meta.music = s
	gm.level:reset_music()
end

function CursorController:room_color(t)
	gm.level.meta.bgcolor = {t[1]/8,t[2]/8,t[3]/8}	
	gm.level:reset()
end

function CursorController:editorsave()
	StringEater({delegate=self, delegateFunction=self.editorsave_result, message="Enter save filename: "}):insert()
end

function CursorController:editorsave_result(_filename)
	fm.lastLaded = _filename
	savelevel(_filename)
end

function CursorController:autosave()
	if _DEBUG and fm.lastLoaded then savelevel(fm.lastLoaded) end
	dialog_text("SAVED")
end

function CursorController:editorload()
	StringEater({delegate=self, delegateFunction=self.editorload_result, message="Enter load filename: "}):insert()
end

if _DEBUG then
	function mcc()
		fm.block_autosave = true
		fm.block_music = true
	end
end

-- TODO this should be in level
function loadent(make)
	if make.special == "player" then
		-- Do nothing!
	elseif make.kind == km.BLOCK then
		local blockAlready = gm.level:find(make.x,make.y,km.BLOCK)
		if blockAlready then destroyEnt(make.x,make.y,blockAlready) end
		local spriteAlready = gm.level:find(make.x,make.y,km.SPRITE)
		if spriteAlready then gm.level:spritePos({e=spriteAlready.e,x=make.x,y=make.y}) end
	
		gm.level:createBlock(make)
	elseif make.kind == km.SPRITE then
		gm.level:createSprite(make)
	elseif make.kind == km.TRIGGER or make.kind == km.ANCHOR then
		gm.level:createSpecial(make)
	end
end

function savelevel(_filename)
	local filename = bridge:editor_dir() .. "/level/" .. _filename .. ".xml"
	local content = {}
	for k,v in gm.level.map:iter() do
		for k2,v2 in pairs(v) do
			if v2.make then
				table.insert(content, v2.make)
			end
		end
	end
	local result = {map=content,meta=gm.level.meta}
	print("SAVING " .. filename)
	bridge:saveTableIntoFile(filename, "level", result)
end

function loadlevel(_filename, anchor)
	local filename = "media/level/" .. _filename .. ".xml"
	if loadlevelfull(filename, anchor) then
		fm.lastLoaded = _filename
		fm.lastAnchor = anchor
	end
end

function loadlevelfull(filename, anchor)
	local t = bridge:loadTableFromFile(filename, false)
	print(string.format("LOADING: %s", filename))
	if not t then print(string.format("NOT FOUND: %s", filename)) return false end

	if gm.level then gm.level:die() end
	gm.level = Level({meta=t.meta})
	
	for i,make in ipairs(t.map) do
		loadent(make)
	end
	
	gm.level:reset()
	gm.level:reset_music()
	
	local where = gm.level.anchors[anchor] or gm.level.anchors[1] or gm.level.cm
	local player = PlayerController({make=where}):insert()
	local recenter_to = player
	
	local recenter
	if where.recenter == nil then
		recenter = gm.level.anchors[anchor] and anchor ~= 1 and not gm.level.meta.camfixed
	else
		recenter = where.recenter
		print(where)
		print(gm.level.anchors[where.recenter_anchor])
		if recenter and where.recenter_anchor then
			local new_where = gm.level.anchors[where.recenter_anchor]
			if new_where then recenter_to = new_where end
		end
	end
	
	if recenter then
		gm.level.cm.x,gm.level.cm.z = recenter_to.x,recenter_to.y
		gm.level:updateCamera()
	end
	
	if _DEBUG then
		gm.level.cursor = CursorController({make={x=1,y=1}}):insert()
	end
	return true
end

function testLevel() -- "Manually specified" level.
	if gm.level then gm.level:die() end
	
	gm.level = Level()
	for x=1,7 do
		for y=1,7 do
			local b = gm.level:createBlock({x=x,y=y})
		end
	end
	PlayerController({make={x=3,y=1}}):insert()
	if _DEBUG then -- Editor
		gm.level.cursor = CursorController({make={x=1,y=1}}):insert()
	end
	gm.level:createSprite({x=2,y=5,sprite="ppl/nellie",flip=true})
	gm.level:reset()
end

-- TODO: More globally accessible
function CursorController:editorload_result(_filename)	
	loadlevel(_filename)
end