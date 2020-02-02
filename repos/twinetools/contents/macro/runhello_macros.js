(function () {
	version.extensions['runhello'] = {
		major: 0,
		minor: 1,
		revision: 1
	};
	
	function varname(str) {
		var splut = str.split(".")
		str = splut[splut.length-1]		
		return str
	}
	
	// Utility macros
	
	// Arbitrary (100 chars) amount of ASCII noise
    macros['noise'] = {
        handler: function (place, macroName, params, parser) {
			var str = "";
			for(var c = 0; c < 100; c++) {
				str = str + String.fromCharCode(Math.floor(Math.random()*96)+32);
			}
			new Wikifier(place, str);
		}
    }
		
	// Macro to loop over a javascript array
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
		handler: function (place, macroName, params, parser) {}
	}
	
	// Functions, can be used in <<set>> or <<print>>
	
	function sax(start, input) { // Support function for roompick
		for(var i = 0; i < input.length; i++) {
			start^=(start<<5) + (start>>2) + input.charCodeAt(i)
			start = start % 256;
		}
		return start
	}
	
	window.roompick = function(name, options) {
		var hash = sax(sax(0, state.history[0].passage.title), name)
		
		return options[ hash % options.length ]
	}
	window.randompick = function(options) {
		return options[Math.floor(Math.random()*options.length)]
	}
	
}());
