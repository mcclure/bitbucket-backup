# Start by running: npm install -g typescript typings && npm install

all: install/app.js install/tsGlue.js install/index.html install/style.css

# Clean.
clean:
	rm -f install/* src/tsGlue.nim tools/dts2nim.js

install/tsGlue.js: src/tsGlue.ts
	mkdir -p $(@D)
	tsc

install/app.js: src/app.nim src/tsGlue.nim
	mkdir -p $(@D)
	nim js -o:$@ $<

install/index.html: static/index.html
	mkdir -p $(@D)
	cp $< $@

install/style.css: static/style.css
	mkdir -p $(@D)
	cp $< $@

src/tsGlue.nim: src/tsGlue.ts install/tsGlue.js tools/dts2nim.js
	node tools/dts2nim.js -q $< > $@

tools/dts2nim.js: tools/typings/index.d.ts tools/dts2nim.ts
	tsc -p tools

tools/typings/index.d.ts: tools/typings.json
	cd tools && typings install
