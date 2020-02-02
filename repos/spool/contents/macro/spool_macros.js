(function () {
	// First load jquery
	var s = document.createElement('script');
	s.src = '/support/jquery-1.9.1.min.js';
	s.onload = function f(){ 
		jQuery.noConflict(); 
	}
	document.body.appendChild(s);

	version.extensions['spool'] = {
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
	
}());
