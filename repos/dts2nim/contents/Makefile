# Start by running: npm install -g typescript typings && npm install

all: bin/dts2nim.js

# Clean

clean:
	rm -f bin/* tests/test.js tests/tsGlue.js tests/tsGlue.nim tests/testMain.js

# Build app

bin/dts2nim.js: typings/index.d.ts src/dts2nim.ts
	mkdir -p $(@D)
	tsc

typings/index.d.ts: typings.json
	typings install

# Tests

test: tests/test.js
	node ./tests/test.js

tests/test.js: tests/tsGlue.js tests/testMain.js
	echo "#!/usr/bin/env node" > $@
	cat tests/tsGlue.js >> $@
	cat tests/testMain.js >> $@

tests/tsGlue.js: tests/tsGlue.ts
	mkdir -p $(@D)
	tsc -p $(@D)

tests/tsGlue.nim: tests/tsGlue.ts tests/tsGlue.js bin/dts2nim.js
	node bin/dts2nim.js -q $< > $@

tests/testMain.js: tests/testMain.nim tests/tsGlue.nim
	mkdir -p $(@D)
	nim js -o:$@ $<

