emily - an ML-y programming language
====================================

## SYNOPSIS

`emily` [<options>] [<input-source>]...

## DESCRIPTION

**emily** is a stand-alone Emily language interpreter. It executes one or more Emily programs from the command line. Unless `-i` is specified, each program will be executed in an isolated scope.

An input source (an Emily program) may be one of the following:

- A filename
- `-e` 'program source here'
- A lone `-`, to receive input from stdin and execute it.

Documentation of the Emily programming language can be found at <br><<http://emilylang.org>> or <<http://bitbucket.org/runhello/emily>>.

## OPTIONS

  * `-e` <source>:
    Execute a program inline.

  * `-i`:
    After executing the programs (if any), boot into interactive mode. Lines of code will be accepted from the command line and their results will be printed.

  * `--package-path` <path>:
    Path to use for "package" loader (default is ../lib/emily/0.3b, relative to the emily interpreter binary).

  * `--project-path` <path>:
    Path to use for "project" loader (default is enclosing directory of source file, or current working directory for a program loaded from `-e` or stdin).

  * `-v`, `--version`:
    Print human-readable interpreter version.

  * `--machine-version`:
    Print machine-readable interpreter version.

  * `-help`, `--help`:
    Show usage message.

## DEBUG OPTIONS

A series of "debug" flags are included, intended for debugging the interpreter. These are not standardized and may vary in future versions of the interpreter. To see a listing, run `emily` with either the `--help` flag or no flags.

## ENVIRONMENT VARIABLES

Some command line arguments may alternately be specified as environment variables. If both environment variable and command line argument are supplied, the command line argument will take precedence.

The mapping is:

* `--package_path` -> `EMILY_PACKAGE_PATH`
* `--project_path` -> `EMILY_PROJECT_PATH`

## EXIT STATUS

The exit status will be 0 if the programs terminated without any errors, and nonzero if a program terminated prematurely.

## PREFERRED PRONOUNS

The preferred pronouns for the Emily interpreter are "it" or "she".

## AUTHOR

Andi McClure <andi.m.mcclure@gmail.com\>