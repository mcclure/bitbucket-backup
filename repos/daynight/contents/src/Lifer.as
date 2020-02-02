package
{
	import flash.display.BitmapData;
	import flash.geom.Rectangle;
	
	import net.flashpunk.Entity;
	import net.flashpunk.graphics.Image;
	
	// Entity for "something that runs a life board". Some swimmer logic has leaked into this.
	public class Lifer extends Entity
	{
		protected var image:Image;
		protected var texture:BitmapData;
		//private var rect:Rectangle;
		protected var frame:int = 0; // "Current time"
		protected var statecount:int = 4;
		protected var colors:Vector.<int> = new <int>[0xFFFFFF, 0, 0xFF0000, 0x7F7FFF];
		protected var rules:Vector.<Vector.<Vector.<int>>>; // From -> countof -> result
		protected var boards:Vector.<Vector.<Vector.<int>>> = new Vector.<Vector.<Vector.<int>>>(2);
		protected var lastReset:int = 0;
		protected var ox:int = 0; // "Offset x", "offset y"-- position of "camera"
		protected var oy:int = 0;
		
		// Takes a rule string like "3478", for each encodes that if you have a cell
		// of state "from", and you count 3, 4, 7, or 8 neighbors of state "counting"
		// (in this example), then move to state "to" in the next generation
		private function loadRule(from:int, counting:int, to:int, cond:String):void {
			var num:int;
			var c:int;
			for(c = 0; c < cond.length; c++) {
				num = cond.charCodeAt(c);
				num -= "0".charCodeAt(0);
				if (num >= 0 && num < 10) {
					rules[from][counting][num] = to;
				}
			}
		}
		
		// Allocate ( but do not populate ) build rule structure
		private function buildRules():void {
			rules = new Vector.<Vector.<Vector.<int>>>;
			for(var z:int = 0; z < statecount; z++) {
				var from:Vector.<Vector.<int>> = new Vector.<Vector.<int>>;
				rules[z] = from;
				for(var y:int = 0; y < statecount; y++) {
					var countof:Vector.<int> = new Vector.<int>;
					from[y] = countof;
					for(var x:int = 0; x < 10; x++) {
						countof[x] = -1;
					}
				}
			}
		}
		
		// Allocate (but populate only to state 0) board structure
		private function buildBoards(width:int, height:int):void {
			for(var c:int = 0; c < 2; c++) {
				var board:Vector.<Vector.<int>> = new Vector.<Vector.<int>>(height);
				boards[c] = board;
				for(var y:int = 0; y < height; y++) {
					var row:Vector.<int> = new Vector.<int>(width);
					board[y] = row;
					for(var x:int = 0; x < width; x++) {
						row[x] = 0;
					}
				}
			}
		}
		
		// Populate board structure with random mix of states 0 and 1
		protected function randomBoard(board:Vector.<Vector.<int>>):void {
			lastReset = frame;
			for(var y:int = 0; y < board.length; y++) {
				var row:Vector.<int> = board[y];
				for(var x:int = 0; x < row.length; x++) {
					row[x] = int(Math.random() > 0.5);
				}
			}
		}
		
		// Whatever it is this class does at startup-- meant for override
		protected function populateBoard(board:Vector.<Vector.<int>>):void {
			randomBoard(board);
		}
		
		// Map a coordinate into the wrapping space.
		protected function wrapx(x:int):int {
			return (x + night.WIDTH*2)%night.WIDTH;
		}
		protected function wrapy(y:int):int {
			return (y + night.WIDTH*2)%night.WIDTH;
		}
		// Map a coordinate into the wrapping space, with the "camera" pixel offset
		protected function visx(x:int):int {
			return wrapx(x + ox);
		}
		protected function visy(y:int):int {
			return wrapy(y + oy);
		}
		
		public function Lifer() 
		{
			super();
			
			// Build and populate rules
			buildRules();
			var saturate:String = "123456789";
			// States 1, 2 and 3, by themselves, evolve according to the query-specified rule
			// and treat all other cell colors as white. Exception: state 3 is floodfilled
			// if it comes into contact with states 1 or 2
			loadRule(0, 1, 1, night.BORN);
			loadRule(1, 1, 1, night.LIVE);
			loadRule(0, 2, 2, night.BORN);
			loadRule(2, 2, 2, night.LIVE);
			loadRule(0, 3, 3, night.BORN);
			loadRule(3, 3, 3, night.LIVE);
			loadRule(3, 2, 2, saturate);
			loadRule(3, 1, 2, saturate);
			
			// Build and populate boards
			buildBoards(night.WIDTH, night.HEIGHT);
			populateBoard(boards[0]);
			
			// Entity drawing setup
			texture = new BitmapData(night.WIDTH,night.HEIGHT,false, 0xFF0000);
			image = new Image(texture);
			image.scale = Number(night.RWIDTH)/night.WIDTH;
			graphic = image;
			//rect = new Rectangle(0,0,night.RWIDTH,night.RHEIGHT);
		}
		
		// For override
		protected function postUpdate(boardTo:Vector.<Vector.<int>>):void
		{
			
		}
		
		override public function update():void 
		{
			var boardFrom:Vector.<Vector.<int>> = boards[frame%2];
			var boardTo:Vector.<Vector.<int>> = boards[(frame+1)%2];
			//var saw : int = 0; // How many black pixels? Not currently used
			var x:int;

			// Iterate over board applying rule + drawing
			for(var y:int = 0; y < night.HEIGHT; y++) {
				for(x = 0; x < night.WIDTH; x++) {
					var here:int = boardFrom[y][x]; // Current state?
					var theserules:Vector.<Vector.<int>> = rules[here]; // Transition rules for current state?
					var there:int = 0; // State to transition to?
					texture.setPixel(visx(x),visy(y),colors[here]); // Draw current state
					
					// For each state that exists, count the number of neighbors of that state
					for(var check:int = 0; check < statecount; check++) {
						var neighbors:int = 0;
						for(var dy:int = -1; dy <= 1; dy++) {
							for(var dx:int = -1; dx <= 1; dx++) {
								if (dx || dy) {
									var fx:int = wrapx(x + dx);
									var fy:int = wrapy(y + dy);
									//trace("x", x, "y", y, "fx", fx, "fy", fy, "board",boardFrom[fy][fx]); 
									neighbors += int(boardFrom[fy][fx] == check);
								}
							}
						}
						// Is there a transition rule for this exact number of neighbors of this state?
						var result:int = theserules[check][neighbors];
						if (result >= 0) {
							there = result;
							break; // We found a transition, we can stop checking
						}
					}
					boardTo[y][x] = there; // If no transition was found this will still be 0
					//saw += int(there);
				}
			}
			
			postUpdate(boardTo); // Give subclasses a chance to do whatever
			
			// In the 2P version this was a "score" bar at the bottom showing who was winning
			//if (night.SCORE) {
			//	var percent : Number = saw/night.HEIGHT; // divided by then times by night.WIDTH
			//	for(x = 0; x < night.WIDTH; x++) {
			//		var color:int = x < percent?0xFF0000:0x0000FF
			//		texture.setPixel(x,night.HEIGHT-1,color);
			//		texture.setPixel(x,night.HEIGHT-2,color);
			//	}
			//}
			
			// Draw
			image.updateBuffer();
			frame++;
		}
	}
}