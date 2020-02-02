This is a rewrite of the old `make` utility, with the intent of retaining the things that keep people coming back to Make while making the experience overall more friendly. Improvements over traditional Make include:

- Build rules are written in Python instead of sh
- More predictable syntax and behavior
- Portable to anywhere Python is (ie: it will work on Windows)
- Easier to make Makefiles modular

If you're not familiar with what Make is: this is a tool designed to let you "build" a software program: compile files, package files into a directory or zip, etc.

[TOC]

# Quick start

This section assumes you have not used any version of `make` before.

## How to install make.py

make.py lives in the repository along with your source files-- your users do not need to install it. (Although your users *will* need to have [Python](http://python.org/), and install `pyparsing` using `sudo easy_install pyparsing`.)

To install it to your repository, just copy the file `make.py` out of this repository into yours.

## How to use make.py

Make.py has a list of *targets* which are things it knows how to build.

If you run `./make.py` by itself, it will build the default target, which is named "all" and usually builds everything.

If you run `./make.py` with one or more arguments, it will build those targets instead. For example `./make.py clean` will run a special target which deletes what you've already built.

Make.py also has a list of *variables* which can change how the build works. You can specify these as environment variables, or on the command line like `./make.py CC=clang`.

## How to write a Make.rules

A file named `Make.rules`, which you should put in the same directory as `make.py`, defines the targets and variables that make.py will act on. The file contains a list of *rules*: Each rule relates a target to a list of *dependencies*, which are targets which must be built first, and a *recipe*, which are instructions to create the target if it doesn't exist.

The `Make.rules` file is written in a simple "makefile language" which only describes the variables and rules. The recipes are written in Python.

Defining a target looks like:

    file1.txt: resources/file2.txt
        copy(target, dep)

Here `file1.txt` is the target, `resources/file2.txt` is its dependency, and `copy(target, dep)` is the Python recipe that creates `file1.txt`. `target` and `dep` are special variables that always contain the current dependency and target; they're used for convenience, and the recipe could just as well have said `copy('file1.txt1', 'resources/file2.txt')`. A couple of things to notice:

* Here both `file1.txt` and `resources/file2.txt` are files. Targets do not have to be files.
* *If* they are files, however, the target will only be built if it is "out of date". If `file1.txt` had no dependencies, then its recipe would only be run if `file1.txt` did not already exist. Since it has dependencies, each of the dependencies will be checked; `file1.txt` will only be built if `file1.txt` is older than `resources/file2.txt`, its source (or if file1 doesn't exist).
* Both the dependency and the recipe are optional; all you actually need to make a rule is the target and the colon. Targets which are not files, like `all`, often have rules consisting of only a target and dependencies (for example: `all: file1.txt file2.txt file3.txt`).

The recipe can contain any normal Python, but there are a few functions already imported that are convenient for Makefiles, like `copy`. This is [FIXME] documented below.

# Reference

## The `make.py` command line tool

[TODO]

## The makefile language

[TODO]

### About internationalization

`Make.rules` files must be UTF-8, or else UTF-16 with a BOM.

## Python recipes

# How to extend make.py

If there's some other place in your repository you'd prefer build scripts live, you can put `make.py` there and copy the file `sample/utility/make.py` into your repository root. This is a trampoline that just re-executes the real `make.py` (you will need to edit the trampoline script to make sure it has the right path).

I recommend [TODO]

# Authorship and license

This was written by Andi McClure <<andi.m.mcclure@gmail.com>> of [http://runhello.com](http://runhello.com)

It is available under the [Creative Commons Zero](http://creativecommons.org/publicdomain/zero/1.0/legalcode.txt) license. In other words, the contents of this directory may be used for any purpose without restriction. If you reshare the make.py repository (as opposed to simply embedding make.py into your own repository), then I would prefer you keep attribution intact-- I think this would be polite. However, there is no legal requirement that you do this.
