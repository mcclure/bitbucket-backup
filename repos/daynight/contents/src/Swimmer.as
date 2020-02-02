package
{
	import net.flashpunk.utils.Input;
	import net.flashpunk.utils.Key;
	
	// Entity for "Lifer with one controllable swimmer and a goal" in it
	public class Swimmer extends Lifer
	{
		protected var px:int; // Player 1 position x,y
		protected var py:int;
		protected var gx:int; // "Goal" position x,y
		protected var gy:int;
		protected var live : Boolean; // Is player 1 being drawn?
		protected var exit : Boolean; // Is goal being drawn?
		protected var deadSince:int = -1; // Has player 1 caught fire? If so, at what frame #?
		protected var winSince:int = -1; // Has player 1 "won"? If so, at what frame #?
		protected var WINSPAN:int; // Pixel span across for "win" animation
		protected var WINLEN:int = 16; // Number of frames "win" animatino lasts
		
		public function Swimmer()
		{
			super();
			px = night.WIDTH/2-night.PWIDTH/2;
			py = night.HEIGHT/2-night.PHEIGHT/2;
			gx = px;
			gy = py;
			live = false;
			exit = false;
			WINSPAN = night.HEIGHT/8;
		}
		
		protected override function populateBoard(board:Vector.<Vector.<int>>):void {
			randomBoard(board);
		}
		
		// Determines if player and exit squares touch. Could this be simpler?
		protected function touching_exit():Boolean {
			var dx:int = px;
			var dy:int = py;
			if (dx-gx < -night.WIDTH/2)
				dx += night.WIDTH;
			else if (dx-gx > night.WIDTH/2)
				dx -= night.WIDTH;
			if (dy-gy < -night.WIDTH/2)
				dy += night.WIDTH;
			else if (dy-gy > night.WIDTH/2)
				dy -= night.WIDTH;
			dx -= gx;
			dy -= gy;
			if (dx<0) dx = -dx;
			if (dy<0) dy = -dy;
			return dx<night.PWIDTH && dy<night.PHEIGHT;
		}
		
		override protected function postUpdate(boardTo:Vector.<Vector.<int>>):void
		{
			var x:int; var y:int; var dx:int; var dy:int; var tries:int; var safe:Boolean;
			var boardFromHasExit:Boolean = live; // Quirk to ensure exit doesn't disappear on wrong frame 
			
			// After player dies, wait around for a second or so. When "death animation" ends:
			if (deadSince >= 0 && frame-deadSince > night.GESTATE) {
				deadSince = -1;
				live = false;
				exit = false;
				randomBoard(boardTo);
			}
			
			// After game resets (because of a death or win) do nothing for a second or so. Then:
			if ((!live || !exit) && (frame-lastReset > night.GESTATE)) {
				// We are ready to place the player, but have not done so yet.
				if (!live) for(tries = 0; !live && tries < 50; tries++) {
					px = Math.random() * night.WIDTH; // Pick a random place... 
					py = Math.random() * night.HEIGHT;
					//trace("try", tries, "px", px, "py", py)
					safe = true; // ...then check if it has enough whitespace around it.
					for(y = -night.CLEARANCE; safe && y < night.CLEARANCE; y++) {
						for(x = -night.CLEARANCE; safe && x < night.CLEARANCE; x++) {
							dx = wrapx(px+x);
							dy = wrapy(py+y);
							if (boardTo[dy][dx])
								safe = false;
						}
					}
					if (safe) {
						trace("Took", frame-lastReset,"frames,",tries,"tries")
						px = wrapx(px - night.PWIDTH/2);
						py = wrapy(py - night.PHEIGHT/2);
						live = true; // If this line isn't hit, we'll try again next frame.
					}
				}
				// We are ready to place the exit, but have not done so yet.
				// This is of course a blatant copy/paste and it's really gross!
				// Will fix when I feel more confident with Actionscript.
				if (live && !exit) for(tries = 0; !exit && tries < 50; tries++) {
					gx = Math.random() * night.WIDTH;
					gy = Math.random() * night.HEIGHT;
					//trace("try", tries, "px", px, "py", py)
					safe = true;
					for(y = -night.CLEARANCE; safe && y < night.CLEARANCE; y++) {
						for(x = -night.CLEARANCE; safe && x < night.CLEARANCE; x++) {
							dx = wrapx(gx+x);
							dy = wrapy(gy+y);
							if (boardTo[gy][gx])
								safe = false;
						}
					}
					if (safe) {
						trace("Took", frame-lastReset,"frames,",tries,"tries")
						gx = wrapx(gx - night.PWIDTH/2);
						gy = wrapy(gy - night.PHEIGHT/2);
						if (!touching_exit()) // Of course don't place exit touching the player.
							exit  = true;
					}
				}
			}
			// Player is being drawn, so draw it and handle its dynamics
			if (live || boardFromHasExit) {
				if (live && deadSince < 0) {
					// Check all input
					dx = 0; dy = 0;
					if (Input.check(Key.RIGHT))
						dx++;
					if (Input.check(Key.LEFT))
						dx--;
					if (Input.check(Key.UP))
						dy--;
					if (Input.check(Key.DOWN))
						dy++;
//					if (Input.check(Key.R))
//						randomBoard(boardTo);
					
					// Move player
					px = wrapx(px + dx);
					py = wrapy(py + dy);
					
					// Move camera (maybe)
					if (visx(px) < night.CAMERABUMP)
						ox = wrapx(ox + 1);
					if (visx(px) >= night.WIDTH-night.CAMERABUMP)
						ox = wrapx(ox - 1);
					if (visy(py) < night.CAMERABUMP)
						oy = wrapx(oy + 1);
					if (visy(py) >= night.HEIGHT-night.CAMERABUMP)
						oy = wrapy(oy - 1);
				}
				
				// Iterate over player pixels and:
				for(y = 0; y < night.PHEIGHT; y++) {
					for(x = 0; x < night.PWIDTH; x++) {
						if (live && deadSince < 0) { // Draw player, possibly kill player
							dx = wrapx(px+x);
							dy = wrapy(py+y);
							
							if (boardTo[dy][dx] == 1 || boardTo[dy][dx] == 2)
								deadSince = frame;
							texture.setPixel(visx(dx),visy(dy),0x0000FF);
							boardTo[dy][dx] = 3;
						}
						
						if (exit) { // Draw exit
							dx = wrapx(gx+x);
							dy = wrapy(gy+y);
							texture.setPixel(visx(dx),visy(dy),0xFF7F00);
							boardTo[dy][dx] = 0; // Exit constantly sweeps out underneath
						}
					}
				}
				
				// Check if player has won
				if (live && deadSince < 0 && touching_exit()) {
					night.PWIDTH++;
					night.PHEIGHT++;
					night.CLEARANCE++;
					live = false;
					exit = false;
					randomBoard(boardTo);
					winSince = frame;
				}
			}
			
			// If player has recently won, draw a little animation
			if (winSince > 0 && frame-winSince < WINLEN) {
				var into:Number = (frame-winSince)/WINLEN;
				for(y = 0; y < 8; y++) {
					var sq:int = into*WINSPAN*y;
					for(x = 0; x < sq; x++) {
						texture.setPixel(visx(px+x),visy(py+sq),0xFF7F00);
						texture.setPixel(visx(px-x),visy(py+sq),0xFF7F00);
						texture.setPixel(visx(px+x),visy(py-sq),0xFF7F00);
						texture.setPixel(visx(px-x),visy(py-sq),0xFF7F00);
						texture.setPixel(visx(px+sq),visy(py+x),0xFF7F00);
						texture.setPixel(visx(px+sq),visy(py-x),0xFF7F00);
						texture.setPixel(visx(px-sq),visy(py+x),0xFF7F00);
						texture.setPixel(visx(px-sq),visy(py-x),0xFF7F00);
					}
				}
			} else {
				winSince = -1;
			}
		}
	}
}