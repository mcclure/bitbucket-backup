import color = require("./color")

let basecanvas = <HTMLCanvasElement> document.getElementById('canvas'),
	basecontext = basecanvas.getContext('2d');
    
let canvas = <HTMLCanvasElement> document.getElementById('canvas'),
    context = canvas.getContext('2d');

let DIAMOND_RADIUS = 44

let DR = DIAMOND_RADIUS

function resizeCanvas() {
    canvas.width = window.innerWidth
    canvas.height = window.innerHeight
    
    Animator.current = new (Animator.current.here())(Animator.current.percent())
    // Changing size will clear canvas
    drawCanvas()
}

function drawDiamond(top?:number, bottom?:number) {
	context.moveTo( -DR, 0   )
	context.beginPath();
	context.lineTo( 0,   -DR )
	context.lineTo( DR,  bottom==null?0:bottom   )
	context.lineTo( 0,   bottom==null?DR:bottom  )
}

class Animator {
	static current: Animator
	offset: number
	max: number
	constructor(percent:number) {
		this.offset = this.max ? Math.min(percent*this.max, this.max-1) : 0
	}
	percent() : number { return this.offset  }
	addOffset(x:number) {
		this.offset += (x > 0 ? 1 : -1)
		this.offsetAdjust()
	}
	offsetAdjust() {
		if (this.offset < 0)
			Animator.current = new (this.prev())(1)
		else if (this.offset >= this.max)
			Animator.current = new (this.next())(0)
	}
	here() : (new (percent:number) => Animator) { return Animator }
	prev() : (new (percent:number) => Animator) { throw new Error("Unimplemented"); }
	next() : (new (percent:number) => Animator) { throw new Error("Unimplemented"); }
	draw(context:CanvasRenderingContext2D) : void { throw new Error("Unimplemented") }
}

class SimpleAnimator extends Animator {
	constructor(percent:number) {
		this.max = 0
		super(percent)
	}
	draw(context:CanvasRenderingContext2D) : void {
		let w = canvas.width, h = canvas.height
		for (let y = 0; y < h; y += DR/2) {
			for (let x = 0; x < w; x += DR/2) {
				context.save()
				context.translate(x,y)
				context.fillStyle = Math.random() > 0.5 ? "black" : "white"
				drawDiamond()
				context.fill()
				context.restore()
			}
		}
	}
	here() : (new (percent:number) => Animator) { return SimpleAnimator }
	prev() : (new (percent:number) => Animator) { return DownAnimator }
	next() : (new (percent:number) => Animator) { return DownAnimator }
}

class DownAnimator extends Animator {
	constructor(percent:number) {
		this.max = Math.floor(canvas.height / (DR/2))
		super(percent)
	}
	draw(context:CanvasRenderingContext2D) : void {
		let w = canvas.width, h = canvas.height
		for (let _y = 0; _y < this.offset; _y++) {
			let y = _y* (DR/2)
			let done = _y+1 == this.offset
			for (let x = 0; x < w; x += DR/2) {
				context.save()
				context.translate(x,y)
				context.fillStyle = Math.random() > 0.5 ? "black" : "white"
				drawDiamond(null, done?h:0)
				context.fill()
				context.restore()
			}
		}
	}
	here() : (new (percent:number) => Animator) { return DownAnimator }
	prev() : (new (percent:number) => Animator) { return this.here() }
	next() : (new (percent:number) => Animator) { return this.here() }
}

Animator.current = new SimpleAnimator(1)

function drawCanvas() {
	Animator.current.draw(context)
}

// Resize the canvas to fill the browser window dynamically
window.addEventListener('resize', resizeCanvas, false)

// Trap attempts at scrolling

$('body').on('mousewheel', function(event:any) {
    Animator.current.addOffset(event.deltaY)
    drawCanvas()
});

let touchid = null
let lasty = null
function touchoff(id, y) : number {
	let isnew = id != touchid
	let off = lasty - y
	lasty = y
	touchid = id
	return isnew ? 0 : off
}
$('body').on('touchstart', function(event:any) {
	event.preventDefault();
	let touch = event.originalEvent.touches[0] || event.originalEvent.changedTouches[0];
	touchoff(touch.identifier, touch.screenY)
})
$('body').on('touchmove', function(event:any) {
	event.preventDefault();
	let touch = event.originalEvent.touches[0] || event.originalEvent.changedTouches[0];
	let off = touchoff(touch.identifier, touch.screenY)
	if (off != 0)
	    Animator.current.addOffset( off )
    drawCanvas()
})

resizeCanvas()
