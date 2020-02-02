# Settings:

VERSION = 0.3b
ifdef EMILY_BUILD_LOCAL
	# When building locally, emily goes in bin/ and support files are thus one level up.
	PREFIX := ..
else
	PREFIX := /usr/local
endif
bindir = $(PREFIX)/bin
libdir = $(PREFIX)/lib
mandir = $(PREFIX)/share/man/man1
INSTALL := install
# TODO: Also delete?
RSYNC := rsync

#TODO: Remove now that we always make a libdir?
CREATE_LIBDIR=1

# Replace "native" with "byte" for debug build
BUILDTYPE=native

PACKAGE_DIR=emily/$(VERSION)

export BUILD_PACKAGE_DIR=$(libdir)/$(PACKAGE_DIR)
export BUILD_INCLUDE_REPL=1
# Another supported option, off by default: BUILD_INCLUDE_C_FFI

# Set this variable to print ocamlbuild sub-commands
ifneq ($(OCAMLBUILD_VERBOSE),)
	OCAMLBUILD_EXTRA = -classic-display -lflag -verbose
endif
OCAML_BUILD = ocamlbuild -no-links -use-ocamlfind $(OCAMLBUILD_EXTRA)

# OCamlbuild will figure out required OCaml files itself.
OCAML_TARGET=src/main.$(BUILDTYPE)
OCAML_EMBED_TARGET=src/main.native.o
OCAML_BUILD_DIR = _build

# Build targets:

.PHONY: all
all: install/bin/emily install/man/emily.1 install/lib/$(PACKAGE_DIR)

# Move final executable in place.
install/bin/emily: $(OCAML_TARGET)
	mkdir -p $(@D)
	cp $(OCAML_BUILD_DIR)/$< $@

# Use ocamlbuild to construct ocaml bits. Always run, ocamlbuild figures out freshness itself.
.PHONY: $(OCAML_TARGET)
$(OCAML_TARGET):
	$(OCAML_BUILD) $@

.PHONY: $(OCAML_EMBED_TARGET)
$(OCAML_EMBED_TARGET):
ifndef BUILD_INCLUDE_C_FFI
	$(error Building the embedded object without including ctypes/libffi-- this will not work correctly. Try building again with BUILD_INCLUDE_C_FFI=1)
endif
	$(OCAML_BUILD) $@

# Move manpage in place
install/man/emily.1: resources/emily.1
	mkdir -p $(@D)
	cp $< $@

# Move packages in place
.PHONY: install/lib/$(PACKAGE_DIR)
install/lib/$(PACKAGE_DIR):
ifdef CREATE_LIBDIR
	mkdir -p $@
	$(RSYNC) -r library/universal/ $@/
ifdef BUILD_INCLUDE_C_FFI
	$(RSYNC) -r library/c-ffi/ $@/
endif
endif

# Embedding support:

# Find OCaml libs
WHERE_OCAML = $(shell ocamlopt -where)
EMBED_OCAML = -L$(WHERE_OCAML) -lasmrun_pic

# If you are on OS X and using Homebrew, ocaml-ctypes will be using Homebrew libffi.
# If this is the case, we need to match it. Check for brew and if we find it use its libffi:
OSX_HOMEBREW_LIBFFI = $(brew --prefix libffi)/lib/libffi.a
ifneq ($(OSX_HOMEBREW_LIBFFI),)
	LIBFFI_LIBS = $(shell brew --prefix libffi)/lib/libffi.a
else
	LIBFFI_LIBS = -Wl,-Bstatic -lffi
endif

# Non-essential: To support tests: build a sample embedded program
.PHONY: test-embed
test-embed: _build_c/embedtester $(OCAML_EMBED_TARGET) install/lib/$(PACKAGE_DIR)
	EMILY_PACKAGE_PATH=install/lib/emily/$(VERSION) ./develop/regression.py -i ./_build_c/embedtester -t sample/test/ffi/regression-ffi.txt

_build_c/ffiEmbedTest.o: src/ffiEmbedTest.c
	mkdir -p $(@D)
	cd $(@D) && $(CC) -c ../$< -std=c99 -Wall

_build_c/embedtester: _build_c/ffiEmbedTest.o $(OCAML_EMBED_TARGET)
	mkdir -p $(@D)
	$(CC) -o $@ _build_c/ffiEmbedTest.o $(OCAML_BUILD_DIR)/$(OCAML_EMBED_TARGET) $(EMBED_OCAML) $(LIBFFI_LIBS) $(LDFLAGS)

# Coding support:

# Non-essential: This prevents ocamlbuild from emitting unhelpful "hints"
_tags:
	touch $@

# Non-essential: Shortcuts for regression test script
.PHONY: test
test:
	./develop/regression.py -a
.PHONY: test-all
test-all:
	./develop/regression.py -A

# Non-essential: Generate man page.
.PHONY: manpage
manpage:
	ronn -r --pipe --manual="Emily programming language" \
	               --date="2015-04-07" \
	               --organization="http://emilylang.org" \
	               doc/manpage.1.md > resources/emily.1

# Install target
.PHONY: install install-makedirs
install-makedirs:
	$(INSTALL) -d $(DESTDIR)$(bindir)
	$(INSTALL) -d $(DESTDIR)$(mandir)
	$(INSTALL) -d $(DESTDIR)$(libdir)/$(PACKAGE_DIR)

install: install-makedirs all
	$(INSTALL) install/bin/emily   $(DESTDIR)$(bindir)
	$(INSTALL) install/man/emily.1 $(DESTDIR)$(mandir)
ifdef CREATE_LIBDIR
	$(RSYNC) -r install/lib/$(PACKAGE_DIR)/ $(DESTDIR)$(libdir)/$(PACKAGE_DIR)
endif

# Clean target
.PHONY: clean
clean:
	ocamlbuild -clean
	rm -f _tags install/bin/emily install/man/emily.1
	rm -rf _build_c/
ifdef CREATE_LIBDIR
	rm -rf install/lib
endif