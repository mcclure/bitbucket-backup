# Settings

VERSION = 0.3b
EMILY_BUILD_LOCAL = 1
ifdef EMILY_BUILD_LOCAL
	PREFIX := .
else
	PREFIX := /usr/local
endif
bindir = $(PREFIX)/bin
libdir = $(PREFIX)/lib
mandir = $(PREFIX)/share/man/man1
INSTALL := install
# TODO: Also delete?
RSYNC := rsync

# FIXME: Is this OS X specific?
# Note relative to src-c dir
INCLUDE=../storage/SDL2.framework/Headers
LDFLAGS=-Finstall -framework SDL2 -Xlinker -rpath -Xlinker '@executable_path/'

#TODO: Remove now that we always make a libdir?
CREATE_LIBDIR=1

PACKAGE_DIR=emily/$(VERSION)

export BUILD_PACKAGE_DIR=$(libdir)/$(PACKAGE_DIR)
export BUILD_INCLUDE_REPL=1
export BUILD_INCLUDE_C_FFI=1

# CTypes support

# Set this variable to print ocamlbuild sub-commands
ifneq ($(OCAMLBUILD_VERBOSE),)
	OCAMLBUILD_EXTRA = -classic-display -lflag -verbose
endif

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

SDL_RULE = install/SDL2.framework/SDL2

# Root names for any C file that we want in here.
C_TARGETS=start sdl util

# OCamlbuild will figure out required OCaml files itself.
OCAML_TARGET=src/main.native.o
OCAML_OBJECT=_build/$(OCAML_TARGET)

C_OBJECTS=$(addprefix src-c/,$(addsuffix .o,$(C_TARGETS)))
C_FILES  =$(C_OBJECTS:.o=.c)
BUILT_OBJECTS = $(OCAML_OBJECT) $(C_OBJECTS)

# Build targets

.PHONY: all
all: install/emily install/lib/$(PACKAGE_DIR) install/game install/resource

# Move final executable in place.
install/emily: $(BUILT_OBJECTS) $(SDL_RULE)
	mkdir -p $(@D)
	$(CC) -o $@ $(BUILT_OBJECTS) $(EMBED_OCAML) $(LIBFFI_LIBS) $(LDFLAGS)

# @D is "directory of target", <F is "filename for first dependency"
$(C_OBJECTS): $(C_FILES)
	cd $(@D) && $(CC) -c $(@F:.o=.c) -I$(INCLUDE) -std=c99 -Wall

# Use ocamlbuild to construct ocaml bits. Always run, ocamlbuild figures out freshness itself.
.PHONY: $(OCAML_OBJECT)
$(OCAML_OBJECT):
	ocamlbuild -no-links -use-ocamlfind $(OCAMLBUILD_EXTRA) $(OCAML_TARGET)

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

.PHONY: install/game
install/game:
ifdef CREATE_LIBDIR
	mkdir -p $@
	$(RSYNC) -r game/ $@/
endif

.PHONY: install/resource
install/resource:
ifdef CREATE_LIBDIR
	mkdir -p $@
	$(RSYNC) -r game-resource/ $@/
endif

$(SDL_RULE):
	mkdir -p install
	$(RSYNC) -r storage/SDL2.framework/ install/SDL2.framework/

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

# Clean target
.PHONY: clean
clean:
	ocamlbuild -clean
	rm -f _tags install/emily install/man/emily.1 src-c/*.o
	rm -rf install/SDL2.framework
ifdef CREATE_LIBDIR
	rm -rf install/lib install/game
endif