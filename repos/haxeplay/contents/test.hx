class Blob {
	public static inline var stdMax = 60;
	public static inline var step = 1;
	var x:Float;
	var y:Float;
	public var i:Int;
	public var max:Int;

	public function new(x:Float, y:Float, ?max:Int) {
		this.x = x; this.y = y; this.i = 10;
		this.max = max==null?stdMax:max;
	}

	public function reMax(max) {
		this.max = max;
	}

	public function draw(context) {
		if (max >= 0 && i > max) return;

      	context.beginPath();
      	context.setFillColor(Math.random(), Math.random(), Math.random(), 1);
      	var i = this.i;
      	if (i >= 0 && i > max/2) i = cast max/2 - (i - max/2);
      	i++;
      	context.fillRect(this.x-i, this.y-i, 2*i, 2*i);
      	this.i += step;
	}
}

class Test {
	static var document:Dynamic;
	static var window:Dynamic;
	static var canvas:Dynamic;
	static var context:Dynamic;

	static var down = false;
	static var blobs = new List();
	static var growblobs = null;

	static public function element(s:String):Dynamic {
		return document.getElementById(s);
	}

	static public function posFor(evt) {
		var rect:Dynamic = canvas.getBoundingClientRect();
		return {
			x: (evt.clientX-rect.left)/(rect.right-rect.left)*canvas.width,
			y: (evt.clientY-rect.top)/(rect.bottom-rect.top)*canvas.height
		};
	}

	static public function draw() {

		while (blobs.first() != null && blobs.first().max >= 0 && blobs.first().i > blobs.first().max)
			blobs.pop();

		for (b in blobs)
			b.draw(context);

		window.requestAnimationFrame(draw);
	}

	static public function onMouseMove(e:Dynamic) {
		var pos = posFor(e);
		var blob = new Blob(pos.x, pos.y, down?-1:null);
		blobs.add(blob);

		if (down) {
			if (growblobs == null) {
				growblobs = new List();
			}
			growblobs.add(blob);
		}
	}

	static public function onMouseDown(e:Dynamic) { if (e.button == 0) down = true; }
	static public function onMouseUp(e:Dynamic) {
		if (e.button == 0) {
			down = false;
			if (growblobs != null) {
				var max = growblobs.first().i*2;
				for (b in growblobs)
					b.reMax(max);
				growblobs = null;
			}
		}
	}

	static public function onLoad(e:Dynamic) {
        canvas = element("canvas");
        context = canvas.getContext("2d");

        canvas.onmousemove = onMouseMove;
		canvas.onmousedown = onMouseDown;
        canvas.onmouseup =   onMouseUp;

        // Initial draw

		window.requestAnimationFrame(draw);
	}

    static public function main() {
    	document = js.Browser.document;
    	window = js.Browser.window;
		window.onload = onLoad;
    }
}