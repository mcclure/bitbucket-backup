(function () {
	// First load jquery
	var s = document.createElement('script');
	s.src = '/support/jquery-1.9.1.min.js';
	s.onload = function f(){ 
		jQuery.noConflict(); 
	}
	document.body.appendChild(s);

	version.extensions['athena'] = {
		major: 0,
		minor: 1,
		revision: 1
	};
	
	// Utility
	function onesplit(str, sep) {
		var at = str.indexOf(sep);
		if (at < -1) return null
		return [str.slice(0,at), str.slice(at+1)]
	}
	function varname(str) {
		var splut = str.split(".")
		str = splut[splut.length-1]		
		return str
	}
	
	// Handler objects. Start with "superclass"
	function Handler() {
	}
	Handler.prototype.macroToSend = function(contents, session) {
		return null
	}
	Handler.prototype.resultsToOutput = function(data, session) {
		return null
	}
	Handler.prototype.handler = function (place, macroName, params, parser) {
		if (jQuery) {
			var tagEnd = parser.source.indexOf('>>', parser.matchStart) + 2;
			var rest = parser.source.slice(tagEnd);
			var session = {}

			var send = this.macroToSend( parser.fullArgs(), session )
			if (session.halt) return
			
			parser.nextMatch = parser.source.length // Skip to end. Continue only if AJAX returns.
			
			var outerThis = this
			if (send != null) send = JSON.stringify(send)
			
			jQuery.post("/data", send, function(data) {
				var output = outerThis.resultsToOutput(data, session)
				if (output) { new Wikifier(place, output); }
				new Wikifier(place, rest);
			}, "json");

		} else {
			new Wikifier(place, ":(");
		}
	}
	
	// -- Examples of handler functions: Send and receive --
	function SendHandler(action) {
		if (action) this.action = action
		else this.action = "send"
	}
	SendHandler.prototype = new Handler()
	SendHandler.prototype.macroToSend = function (contents, session) {
		var splut = onesplit(contents, "=")
		if (splut) {
			var name = splut[0].trim()
			var val = eval(Wikifier.parse(splut[1]))
			return {
				action: this.action,
				target: name,
				value: val
			}
		}
		session.halt = true
		return null
	}
	
	function ReceiveHandler(action) {
	}
	ReceiveHandler.prototype = new Handler()
	ReceiveHandler.prototype.macroToSend = function (contents, session) {
		var splut = onesplit(contents, "=")
		if (splut) {
			session.local = Wikifier.parse(splut[0])
			var target = splut[1].trim()
			return {
				action: "receive",
				target: target,
			}
		}
		session.halt = true
		return null
	}
	ReceiveHandler.prototype.resultsToOutput = function(data, session) {
		var splut = varname(session.local)
		state.history[0].variables[splut] = data.value
		return null
	}
	
	// -- Room handler --

	var roomState = {room:{},remote:{},piano:{}}
	
	function RoomHandler() {
	}
	RoomHandler.prototype = new Handler()
	RoomHandler.prototype.basicMacro = function() {
		return {
			action: this.action,
			id:roomState.room.id,
		}
	}
	RoomHandler.prototype.macroToSend = function (contents, session) {
		return this.basicMacro()
	}
	
	function CorpseHandler() {
		this.action = "corpse"
	}
	CorpseHandler.prototype = new RoomHandler()
	CorpseHandler.prototype.macroToSend = function (contents, session) {
		var basicMacro = this.basicMacro()
		basicMacro.body = state.history[0].variables.body
		return basicMacro
	}
	
	function EatCorpseHandler() {
		this.action = "eatcorpse"
	}
	EatCorpseHandler.prototype = new RoomHandler()
	EatCorpseHandler.prototype.macroToSend = function (contents, session) {
		var basicMacro = this.basicMacro()
		basicMacro.target = eval(Wikifier.parse(contents))
		return basicMacro
	}
	
	function PianoHandler(action) {
	}
	PianoHandler.prototype = new Handler()
	PianoHandler.prototype.macroToSend = function (contents, session) {
		var basicMacro = {
			action: "piano",
		}
		basicMacro.body = state.history[0].variables.body
		
		return basicMacro
	}
	PianoHandler.prototype.resultsToOutput = function(data, session) {
		if (data.time) {
			roomState.piano.body = null
			roomState.piano.last = data.time
		}
		
		return null
	}
	
	function RoomlookHandler(action) {
		this.action = "roomlook"
	}
	RoomlookHandler.prototype = new RoomHandler()
	RoomlookHandler.prototype.macroToSend = function (contents, session) {
		var basicMacro = this.basicMacro()
		basicMacro.kind = roomState.room.kind
		if (roomState.piano.last)
			basicMacro.time = roomState.piano.last;
		return basicMacro
	}
	RoomlookHandler.prototype.resultsToOutput = function(data, session) {
		roomState.remote = data
		
		if (state.history[0].variables.dangerprotect) { // hax for debugging
			roomState.remote.danger = state.history[0].variables.danger
			state.history[0].variables.dangerprotect = false
		}
		
		if (data.piano && data.piano.length>0) {
			roomState.piano.body = data.piano[0]
			roomState.piano.last = roomState.piano.body.time
		} else {
			roomState.piano.body = null // Possibly clear out old body
		}
		
		return null
	}
	
	function HopeHandler() {
	}
	HopeHandler.prototype = new Handler()
	HopeHandler.prototype.macroToSend = function (contents, session) {
		var basicMacro = {
			action: "hope",
			hope:state.history[0].variables.newhope
		}
		return basicMacro
	}
	
	// Turn handlers into macros	
	var receiveHandler = new ReceiveHandler()
	macros['receive'] = {
		handler: function (place, macroName, params, parser) { receiveHandler.handler(place,macroName,params,parser) }
	}
	
	var sendHandler = new SendHandler()
	macros['send'] = {
		handler: function (place, macroName, params, parser) { sendHandler.handler(place,macroName,params,parser) }
	}

	var addHandler = new SendHandler("accumulate")
	macros['add'] = {
		handler: function (place, macroName, params, parser) { addHandler.handler(place,macroName,params,parser) }
	}
	
	// Room related macros
	window.roomState = roomState
	
	var roomlookHandler = new RoomlookHandler()
	macros['room'] = {
		handler: function (place, macroName, params, parser) {
			var kind = params.length>0 ? params[0] : ""
			roomState.room = {id:state.history[0].passage.title, kind:kind}
			roomlookHandler.handler(place,macroName,params,parser)
		}
	}
	
	var corpseHandler = new CorpseHandler()
	macros['corpse'] = {
		handler: function (place, macroName, params, parser) { corpseHandler.handler(place,macroName,params,parser) }
	}
	
	var eatCorpseHandler = new EatCorpseHandler()
	macros['eatcorpse'] = {
		handler: function (place, macroName, params, parser) { eatCorpseHandler.handler(place,macroName,params,parser) }
	}

	var pianoHandler = new PianoHandler()
	macros['piano'] = {
		handler: function (place, macroName, params, parser) { pianoHandler.handler(place,macroName,params,parser) }
	}
	
	var hopeHandler = new HopeHandler()
	macros['hope'] = {
		handler: function (place, macroName, params, parser) { hopeHandler.handler(place,macroName,params,parser) }
	}
	
	// e : macroName
	// b : parser
	// k : afterTagIdx
	// a : afterTag
	// d : endAt
	// c : tagInner
	// l : depth
	// el : macroNameLen
	// Utility macros
	macros['for'] = { // Loop over a list. Incorporates sections of L's "replace" macro
		handler: function (place, macroName, params, parser) {
			var afterTagIdx = parser.source.indexOf('>>', parser.matchStart) + 2;
            var afterTag = parser.source.slice(afterTagIdx);
            var endAt = -1;
            var tagInner = '';
            var depth = 0;
            var macroNameLen = macroName.length
            for(var i = 0; i < afterTag.length; i++) {
                if(afterTag.substr(i, 7 + macroNameLen) == '<<end' + macroName + '>>') {
                    if(depth == 0) {
                        endAt = afterTagIdx + i + 7 + macroNameLen;
                        break;
                    }
                    else {
                        depth--;
                        tagInner += afterTag.charAt(i);
                    }
                }
                else {
                    if(afterTag.substr(i, 2 + macroNameLen) == '<<' + macroName) {
                        depth++;
                    }
                    tagInner += afterTag.charAt(i);
                }
            }
			var rest = parser.source.slice(endAt)
			
			var into = varname(Wikifier.parse(params[0]))
			var over = eval(Wikifier.parse(params[1]))

			if (over) {
				var before = state.history[0].variables[into]
				for(var i = 0; i < over.length; i++) {
					state.history[0].variables[into] = over[i]
					new Wikifier(place, tagInner);
				}
				state.history[0].variables[into] = before
			}
			
			new Wikifier(place, rest);
			
			parser.nextMatch = parser.source.length // We exhausted the string, so halt this parser.
		}
	}
	macros['endfor'] = {
		handler: function (place, macroName, params, parser) { sendHandler.handler(place,macroName,params,parser) }
	}
	
	// Ooh big hunk of game logic here. reads $corpse and $traits and $body leaves $trait
	macros['trait'] = {
		handler: function (place, macroName, params, parser) {
			var body = state.history[0].variables.body
			var corpse = state.history[0].variables.corpse
			var traits = state.history[0].variables.traits
			var candidates = []
			
			function real(t) {
				return t
			}
			
			for(var i = 0; i < traits.length; i++) {
				var test = traits[i]
				if (real(corpse[test]) && !real(body[test]))
					candidates.push(test)
			}
			
			var result = undefined
			if (candidates.length > 0)
				result = randompick(candidates)
			state.history[0].variables.trait = result
		}
	}
	
	// User-visible utility functions
	function sax(start, input) {
		for(var i = 0; i < input.length; i++) {
			start^=(start<<5) + (start>>2) + input.charCodeAt(i)
			start = start % 256;
		}
		return start
	}
	window.roompick = function(name, options) {
		var hash = sax(sax(0, roomState.room.id), name)
		
		return options[ hash % options.length ]
	}
	window.randompick = function(options) {
		return options[Math.floor(Math.random()*options.length)]
	}
	
}());


