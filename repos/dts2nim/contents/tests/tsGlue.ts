// TypeScript side of tests-- defines functions called by testMain.nim

function testAssert(condition: boolean, message:string) {
	if (!condition)
		throw new Error("Test failed: " + message)
}

function consoleLog(message: string) {
	console.log(message)
}

let QBnumber = 2
let QBstring = "OK"

function QFaddone(x:number) {
	return x + 1
}

class QCorder {
	grand: QCgrand
	constructor() {
		this.grand = new QCgrand("silver")
	}
}

class QCbase {
	num1:number
	str2:string

	add2(x:number) { return x + 2 }
}

class QCchild extends QCbase {
	num3:number
	str4:string
	constructor(ok:string) {
		super()
		this.num3 = 3
		this.str4 = ok
	}
	addNum(y:number) { this.num3 += y }

	static numStatic1: number = 60
	static numStatic2(q: number) {
		return 70 + q
	}
}

class QCgrand extends QCchild {
	addStr(y:string) { return this.str4 + y }
}

let QIchild = new QCchild("3")

let QAarray : QCbase[]

interface QCinterface {
	str5: string
}

let QIinterface: QCinterface = {str5: "five"}

class QCgeneric<T> {
	x: T
}

let QIgeneric : QCgeneric<number>

class QCrecursive1 {
	a1: QCrecursive2
	constructor(public num6: number) {}
}

class QCrecursive2 {
	constructor(public a2 : QCrecursive1) {
		a2.a1 = this
	}
}

class QCrecursive3 {
	num7: number
	constructor(v: QCrecursive4 = null) {
		if (v)
			this.num7 = v.num7
	}
}

class QCrecursive4 extends QCrecursive3 {
	constructor(v2: number) {
		super()
		this.num7 = v2
	}
}

let QIrecursive2 = new QCrecursive2(new QCrecursive1(31))

let QFvaraddten = (x:number) => x + 10

let QFvarcallback : (x: number, y: (z: number) => number) => number

QFvarcallback = (x, y) => y(x + 1) + 1

// This will allow QFbackflow through the type checker, but will not declare a variable
declare var QBbackflow : number

let QFbackflow = () : number => QBbackflow

// A pattern lib.d.ts frequently follows.
interface QCproto {
	num5: number
	func6(v: number) : number
}

interface QCprototheconstructor {
	new (value: number): QCproto
	prototype: QCproto
	numStatic3: number
	numStatic4(q: number) : number
}

class QCimplforproto implements QCproto {
	constructor(public num5: number) {}
	func6(v:number) { return this.num5 + v }
	static numStatic3: number = 80
	static numStatic4(q: number) {
		return 90 + q
	}
}

var QCproto: QCprototheconstructor = QCimplforproto

var QIproto: QCproto = new QCproto(62)

var QFfexample : string | (number | QCproto)

function QFforcenumber(num : string | number) : number {
	return +num
}

interface QCindex {
	[index: number]: number;
}

let QIindex : QCindex = {}
