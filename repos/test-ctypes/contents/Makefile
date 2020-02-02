## Variables to overload

# Find basic tools
bindir = $(PREFIX)/bin
libdir = $(PREFIX)/lib
mandir = $(PREFIX)/share/man/man1
CC = gcc
AR = ar
INSTALL := install

# Set this variable to print ocamlbuild sub-commands, ar files etc
ifneq ($(BUILD_VERBOSE),)
	OCAMLBUILD_EXTRA = -classic-display -lflag -verbose
	AR_EXTRA = v
endif

# Find OCaml libs
WHERE_OCAML = $(shell ocamlopt -where)

## Build products
## In general, the goal here is to use ocamlbuild for anything ocamlbuild
## can in principle handle, and let make handle everything else.
## Things ocamlbuild isn't very good at:
## - Copying files
## - As of 4.02.02, making libraries does not work very well.

# If you are on OS X and using Homebrew, ocaml-ctypes will be using Homebrew libffi.
# If this is the case, we need to match it. Check for brew and if we find it use its libffi:
OSX_HOMEBREW_LIBFFI = $(shell brew --prefix libffi)
ifneq ($(OSX_HOMEBREW_LIBFFI),)
	LIBFFI_LIBS = $(shell env PKG_CONFIG_PATH=$(OSX_HOMEBREW_LIBFFI)/lib/pkgconfig pkg-config --libs libffi)
else
	LIBFFI_LIBS = -lffi
endif

.PHONY: all
all: install/lib/libemily_static.a install/lib/libemily.dylib

# Root names for any C file that we want in here.
C_FILES=cApi cStart cHelper

# OCamlbuild will figure out required OCaml files itself.
OCAML_OBJECT=src/main.native.o

OBJECTS=$(OCAML_OBJECT) $(addprefix src/,$(addsuffix .o,$(C_FILES)))
BUILT_OBJECTS = $(addprefix _build/,$(OBJECTS))

install/lib/libemily_static.a: $(BUILT_OBJECTS)
	mkdir -p $(@D)
	$(AR) rs$(AR_EXTRA) $@ $^

install/lib/libemily.dylib:
	true

# Always run, ocamlbuild figures out freshness itself.
.PHONY: $(BUILT_OBJECTS)
$(BUILT_OBJECTS):
	ocamlbuild -no-links -use-ocamlfind $(OCAMLBUILD_EXTRA) $(OBJECTS)

# Install target
.PHONY: install install-makedirs
install-makedirs:
	$(INSTALL) -d $(DESTDIR)$(bindir)

install: install-makedirs all
	$(INSTALL) install/ctest

# Clean target
.PHONY: clean
clean:
	ocamlbuild -clean
	rm -f install/bin/* install/lib/*
