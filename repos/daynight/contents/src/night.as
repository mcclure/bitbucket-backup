package
{
	import flash.display.StageAlign;
	import flash.external.*;
	
	import net.flashpunk.Engine;
	import net.flashpunk.FP;
	
	public class night extends Engine
	{
		// Constants (all can be set as query args)
		static public var RWIDTH  : int = 512; // "Real width"
		static public var RHEIGHT : int = 512;
		static public var WIDTH   : int = 128; // In-game width
		static public var HEIGHT  : int = 128;
		static public var PWIDTH  : int = 2; // Player width
		static public var PHEIGHT : int = 2;
		static public var SCORE   : int = 0;
		static public var LIVE    : String = "34678";
		static public var BORN    : String = "3678";
		static public var GESTATE : int = 60;
		public static var CLEARANCE : int = 5; 
		public static var CAMERABUMP : int; 
		public static var FPS : int = 30;
		static public var query   : Object;

		public function night()
		{
			// First load static query args
			loadQuery();
			
			if (query.hasOwnProperty("width"))
				WIDTH = int(query["width"]);
			if (query.hasOwnProperty("height"))
				HEIGHT = int(query["height"]);
			if (query.hasOwnProperty("playerwidth"))
				PWIDTH = int(query["playerwidth"]);
			if (query.hasOwnProperty("playerheight"))
				PHEIGHT = int(query["playerheight"]);
			if (query.hasOwnProperty("score"))
				SCORE = int(query["score"]);
			if (query.hasOwnProperty("clearance"))
				CLEARANCE = query["clearance"];
			if (query.hasOwnProperty("live"))
				LIVE = query["live"];
			if (query.hasOwnProperty("born"))
				BORN = query["born"];
			CAMERABUMP = WIDTH/4;
			if (query.hasOwnProperty("camerabump"))
				CAMERABUMP = query["camerabump"];
			if (query.hasOwnProperty("fps"))
				FPS = query["fps"];
			
			super(RWIDTH, RHEIGHT, FPS, false);
		}
		
		// Parse query arguments into 'query' hash
		public function loadQuery():void
		{
			var url:String = ExternalInterface.call("window.location.href.toString");
			trace("Called as ", url);
			query = new Object;
			var splitUrl:Array = url.split("?",2);
			if (splitUrl.length > 1) {
				var arg:Array = splitUrl[1].split("&");
				for(var c:int = 0; c < arg.length; c++) {
					var kv:Array = arg[c].split("=",2);
					if (kv.length > 1) {
						query[kv[0]] = kv[1];
					}
				}
			}
		}
		
		override public function init():void 
		{
			trace("FlashPunk has started successfully!");
			
			FP.world = new LifeWorld;
		}
		
		override public function setStageProperties():void
		{
		    super.setStageProperties();
		    
		    stage.align = StageAlign.TOP; // No center. This is sorta annoying
		}
	}
}