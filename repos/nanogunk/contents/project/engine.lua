-- SDL wrapper
module("engine", package.seeall)

ffi = require( "ffi" )
sdl = require( "ffi/sdl" )

local bit = require("bit")

local shl, shr, bor, band = bit.lshift, bit.rshift, bit.bor, bit.band, math.min, math.max
local min, max = math.min, math.max

local SDL_INIT_AUDIO		= 0x00000010
local SDL_INIT_VIDEO		= 0x00000020
sdl.SDL_Init(bit.bor(SDL_INIT_VIDEO, SDL_INIT_AUDIO)) -- Do early to get video info

local video_info = sdl.SDL_GetVideoInfo()
fullscreenw, fullscreenh = tonumber(video_info.current_w), tonumber(video_info.current_h)
sw, sh = fullscreenw>0 and math.floor(fullscreenw*0.75) or 800, -- fallback if this fails?
	fullscreenh>0 and math.floor(fullscreenh*0.75) or 600
ticks = 0

on_esc = nil

--local SDL_INIT_AUDIO = 0x00000010
--local SDL_INIT_VIDEO = 0x00000020
--sdl.SDL_Init( bit.bor(SDL_INIT_AUDIO, SDL_INIT_VIDEO) )

screen = sdl.SDL_SetVideoMode( sw, sh, 32, sdl.SDL_RESIZABLE )
event, rect, rect2 = ffi.new( "SDL_Event" ), ffi.new( "SDL_Rect" ), ffi.new( "SDL_Rect" )

sdl.SDL_EnableKeyRepeat( sdl.SDL_DEFAULT_REPEAT_DELAY, sdl.SDL_DEFAULT_REPEAT_INTERVAL )
sdl.SDL_EnableUNICODE(1)

function read_text_file(n)
   local lines = {}
   for line in io.lines(n) do
      lines[ #lines + 1 ] = line
   end
   return lines
end

--[[ -- These were *ridiculous* slow
channelname = {"r","g","b"}
local channels = {r=screen.format.Rshift, g=screen.format.Gshift, b=screen.format.Bshift}
function color_x(c,channel)
	return bit.band( bit.rshift( c, channels[channel] ), 0xFF )
end
function color_xset(color,v,channel)
	local shift = channels[channel]
	return bit.bor( bit.band( color, bit.bnot(bit.lshift( 0xFF, shift )) ),
		bit.lshift( v, shift ) )
end
--]]

function color_r(c)
	return bit.band( bit.rshift( c, screen.format.Rshift ), 0xFF )
end
function color_g(c)
	return bit.band( bit.rshift( c, screen.format.Gshift ), 0xFF )
end
function color_b(c)
	return bit.band( bit.rshift( c, screen.format.Bshift ), 0xFF )
end
function color_a(c)
	return bit.band( bit.rshift( c, screen.format.Ashift ), 0xFF )
end
function color_make(r,g,b,a)
	return bit.bor(
		bit.lshift( r, screen.format.Rshift ),
		bit.lshift( g, screen.format.Gshift ),
		bit.lshift( b, screen.format.Bshift )
--		, bit.lshift( a or 0xFF, screen.format.Ashift 
	)
end
function color_gray(g)
	return color_make(g,g,g)
end
white = color_gray(0xFF)
black = color_gray(0x00)

function surfaceSized(w,h)
	return sdl.SDL_CreateRGBSurface(0, w, h, 32, screen.format.Rmask, screen.format.Gmask, screen.format.Bmask, 0)
end

--fw, fh = 7, 12
fw, fh = 14, 24
local font = project.load_image("resource/font/font14x24.png")

local should_exit = false

ui_state = { 
   mouse_down = false, 
   mouse_x = 0, mouse_y = 0, 
   hot_item = 0, active_item = 0,
   kbd_item = 0, pressed={}, down={},  key_mod = false, 
   last_widget = 0
}

function rect_to_string(r)
	return string.format("[rect x %d y %d w %d h %d]", r.x,r.y,r.w,r.h)
end

function draw_charcode( charcode, x, y )
   rect.x,   rect.y,  rect.w,  rect.h = 0, (charcode - 32) * fh, fw, fh
   rect2.x, rect2.y, rect2.w, rect2.h = x, y, fw, fh
   sdl.SDL_FillRect( screen, rect2, black )
   sdl.SDL_UpperBlit( font, rect, screen, rect2 )
end

function draw_string( s, x, y )
   for i=1, #s do
      draw_charcode( s:byte(i), x, y )
      x = x + fw
      if x >= sw then
	 break
      end
   end
end

function draw_string_right( s, x, y )
	draw_string( s, x-#s*fw, y )
end

function clamp(minimum,v,maximum)
   return max(minimum, min(maximum, v))
end

function draw_text( x, y, lines, top, count )
   if type(lines)=="string" then
      lines = { lines }
   end
   local lc = #lines
   top = clamp(1, top or 1, lc)
   local bottom = top + (count or lc)
   bottom = clamp(top, bottom or lc, lc)
   for i = top, bottom do
      draw_string( lines[i], x, y )
      y = y + fh
   end
end

function draw_rect( x, y, w, h, color, surface )
   rect.x, rect.y, rect.w, rect.h = x, y, w, h
   sdl.SDL_FillRect( surface or screen, rect, color )
end

function clear_screen()
	draw_rect( 0, 0, sw, sh, 0, screen )
end

function blit_whole( to, from, x, y )
	rect.x,   rect.y,  rect.w,  rect.h = 0, 0, from.w, from.h
	rect2.x, rect2.y, rect2.w, rect2.h = x or 0, y or 0, from.w, from.h
	sdl.SDL_UpperBlit( from, rect, to, rect2 )
end

function blit_part( to, from, x, y, xf, yf )
	rect.x,   rect.y,  rect.w,  rect.h = xf or 0, yf or 0, from.w, from.h
	rect2.x, rect2.y, rect2.w, rect2.h = x or 0, y or 0, from.w, from.h
	sdl.SDL_UpperBlit( from, rect, to, rect2 )
end

function fix_clip()
	rect2.x, rect2.y, rect2.w, rect2.h = 0,0, screen.w,screen.h
	sdl.SDL_SetClipRect(screen, rect2)
end

function blit_whole_scaled( to, from, scale, x, y )
	scale = scale or 1
	rect.x,   rect.y,  rect.w,  rect.h = 0, 0, from.w, from.h
	rect2.x, rect2.y, rect2.w, rect2.h = x or 0, y or 0, from.w*scale, from.h*scale
	sdl.SDL_UpperBlitScaled( from, rect, to, rect2 )
	fix_clip()
end

function region_hit( x, y, w, h )
   return ( ui_state.mouse_x >= x and
	    ui_state.mouse_y >= y and
	    ui_state.mouse_x <= x + w and
	    ui_state.mouse_y <= y + h )
end

function render(screen) -- TO OVERLOAD
end
function update(ui_state)
end

local function handle( event, handlers )
   if handlers[event] then
      return handlers[event]()
   end
end

local target_fps = 60
local target_tpf = 1000/target_fps -- i.e. ticks per frame

function loop()
   local last_frame_at = sdl.SDL_GetTicks()
   evt = ffi.new( "SDL_Event" )
   evt.type = sdl.SDL_VIDEORESIZE
   evt.resize.w, evt.resize.h = sw, sh
   sdl.SDL_PushEvent( evt )
   while evt.type ~= sdl.SDL_QUIT do
      while sdl.SDL_PollEvent( evt ) ~= 0 do
	 local type, scancode, key, mod = evt.type, tonumber(evt.key.keysym.scancode), evt.key.keysym.sym, evt.key.keysym.mod
	 local motion, button = evt.motion, evt.button.button
 	 handle(
	    type, {
	       [sdl.SDL_VIDEORESIZE]=
	       function()
		  sw, sh = evt.resize.w, evt.resize.h
		  screen = sdl.SDL_SetVideoMode( evt.resize.w, evt.resize.h, 32, sdl.SDL_RESIZABLE )
	       end,

	       [sdl.SDL_QUIT]=
	       function()
		  should_exit = true
	       end,
	       
	       [sdl.SDL_MOUSEMOTION]=
	       function()
		  ui_state.mouse_x = motion.x
		  ui_state.mouse_y = motion.y
	       end,
	       
	       [sdl.SDL_MOUSEBUTTONDOWN]=
	       function()
		  if button == 1 then
		     ui_state.mouse_down = true
		  end
	       end,
	       
	       [sdl.SDL_MOUSEBUTTONUP]=
	       function()
		  if button == 1 then
		     ui_state.mouse_down = false
		  end
	       end,
	       
	       [sdl.SDL_KEYDOWN]= -- TODO: Listeners
	       function()
		     if not ui_state.down[scancode] then
				if key == sdl.SDLK_ESCAPE then
					if on_esc then
						table.insert(doom, on_esc)
						on_esc = nil
					else
						evt.type = sdl.SDL_QUIT
						sdl.SDL_PushEvent( evt )
					end
				end
			   ui_state.pressed[scancode] = true
			   ui_state.down[scancode] = true
			 end
		  ui_state.key_mod = mod
	       end,
	       
	       [sdl.SDL_KEYUP]=
	       function()
			ui_state.down[scancode] = nil
	       end,
	    }
	 )
      end
      render(screen)
	  update(ui_state)
	  
	  ui_state.pressed = {}
	  ticks = ticks + 1
	  
	  local frame = sdl.SDL_GetTicks()
	  local need_delay = target_tpf - (frame-last_frame_at)
	  if need_delay > 0 then sdl.SDL_Delay(need_delay) end
	  last_frame_at = frame
   end
end

local function quit()
	sdl.SDL_Quit()
end

return m