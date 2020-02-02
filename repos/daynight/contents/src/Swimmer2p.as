package
{
	import net.flashpunk.utils.Input;
	import net.flashpunk.utils.Key;
	
	// Entity for "Lifer with two controllable swimmers" in it. May not work in current rev
	public class Swimmer2p extends Swimmer
	{
		protected var px2:int;
		protected var py2:int;
		
		public function Swimmer2p()
		{
			super();
			px = night.HEIGHT/4-night.PWIDTH/2;
			px2 = 3*night.HEIGHT/4-night.PWIDTH/2;
			py2 = night.WIDTH/2-night.PHEIGHT/2;
		}
		
		// In 2P mode, drawn left hand of screen all white and right hand of screen all black.
		// "Perforate" so it will be stable in daynight pattern
		protected override function populateBoard(board:Vector.<Vector.<int>>):void {
			for(var y:int = 0; y < board.length; y++) {
				var row:Vector.<Boolean> = board[y];
				for(var x:int = 0; x < row.length; x++) {
					row[x] = (x+(y%2))%night.WIDTH >= night.WIDTH/2;
				}
			}
		}
		
		override protected function postUpdate(boardTo:Vector.<Vector.<int>>):void
		{
			super.postUpdate(boardTo);
			
			if (Input.check(Key.RIGHT))
				px2++;
			if (Input.check(Key.LEFT))
				px2--;
			if (Input.check(Key.UP))
				py2--;
			if (Input.check(Key.DOWN))
				py2++;
			px2 = wrapx(px2);
			py2 = wrapy(py2);
			
			for(var y:int = 0; y < night.PHEIGHT; y++) {
				for(var x:int = 0; x < night.PWIDTH; x++) {
					var dx:int = wrapx(px2+x);
					var dy:int = wrapy(py2+y);
					
					texture.setPixel(visx(dx),visy(dy),0x0000FF);
					boardTo[dy][dx] = false;
				}
			}
		}
	}
}