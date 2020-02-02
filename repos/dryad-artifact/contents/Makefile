# Script assumes that tsd, typescript, and webpack are already installed.
# Try: npm install -g typescript tsd webpack ts-loader
all: install/app.js install/index.html

# Run once at initial project checkout.
configure: install/index.html install/app.js
	npm install && tsd link && tsd install
	npm link typescript # Prints

# Clean.
clean:
	rm -f build/* install/*

install/app.js: src/app.ts
	mkdir -p $(@D)
	webpack

install/index.html: index.html
	mkdir -p $(@D)
	cp $< $@
