# Start by running: npm install -g typescript && npm install

DTS2NIM = ./node_modules/.bin/dts2nim

all: js

js: install/app.js install/tsGlue.js install/index.html install/style.css lib/tone-0.7.1/Tone.min.js

c: desktop/app

# Clean.
clean:
	rm -f install/* src/tsGlue.nim desktop/app

# JS

install/tsGlue.js: src/tsGlue.ts
	mkdir -p $(@D)
	tsc

install/app.js: src/app.nim src/glue.nim src/tsGlue.nim
	mkdir -p $(@D)
	nim js -o:$@ $<

install/index.html: static/index.html
	mkdir -p $(@D)
	cp $< $@

install/style.css: static/style.css
	mkdir -p $(@D)
	cp $< $@

lib/tone-0.7.1/Tone.min.js:
	mkdir -p $(@D)
	curl -o $@ https://raw.githubusercontent.com/Tonejs/Tone.js/5b279569e6701c488fc321c471f2a6edcf45c6bb/build/Tone.min.js 

# Depend on dts2nim binary to force a rebuild after an upgrade
src/tsGlue.nim: src/tsGlue.ts install/tsGlue.js $(DTS2NIM)
	$(DTS2NIM) $< -o $@ -q --number

$(DTS2NIM):
	@echo "ERROR: dts2nim binary is missing! Please run 'npm install'." >&2
	@exit 1

# C

# --tlsEmulation:off seems to be needed to make SDL+threads on Windows not crash
# It seems to only be available in Nim 0.13.1 (current beta version), however
desktop/app: src/app.nim src/glue.nim src/sdlGlue.nim
	mkdir -p $(@D)
	nim c --threads:on --tlsEmulation:off -o:$@ $<
