# Start by running: npm install -g typescript && npm install

DTS2NIM = ./node_modules/.bin/dts2nim

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

# Depend on dts2nim binary to force a rebuild after an upgrade
src/tsGlue.nim: src/tsGlue.ts install/tsGlue.js $(DTS2NIM)
	$(DTS2NIM) $< -o $@ -q

tools/dts2nim.js: tools/dts2nim.ts
	tsc -p tools

$(DTS2NIM):
	@echo "ERROR: dts2nim binary is missing! Please run 'npm install'." >&2
	@exit 1
