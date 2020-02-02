ppx_const
=========

This is an OCaml language extension implementing an `if%const` statement. The `if%const` is evaluated at compile time, and the appropriate clause substituted without the ignored clause being fully compiled. This allows you to avoid consequences such as module inclusion or type inference changes which would otherwise have resulted from the ignored clause.

In other words, ppx\_const works like `#if` in the C preprocessor, but is implemented entirely within the OCaml language using the [ppx](http://whitequark.org/blog/2014/04/16/a-guide-to-extension-points-in-ocaml/) mechanism. In conjunction with [ppx_getenv](https://github.com/whitequark/ppx_getenv), this provides a lightweight alternative to Cppo.

This software was written by Andi McClure <<andi.m.mcclure@gmail.com>> based on whitequark's ppx\_getenv sample. Because it is based on ppx, it requires OCaml 4.02.

Usage
-----

ppx\_const may be invoked with either of the following:

    if%const COND then A else B
    if%const COND then A

COND must be one of the following:

* `true`
* `false`
* An expression consisting of two literals and either the `<>` or `=` operator.

COND may also contain extension nodes (including `if%const`s) as long as they evaluate to a constant expression by the time ppx\_const sees them.

A and B are not required to be of the same type. Like with normal `if`, the return type of `if%const false then X` is unit.

An example: Using ppx_const with ppx\_gentenv
---------------------------------------------

Say your program has a Graph module with heavyweight dependencies (cairo or something). Some users may prefer to compile your program without the graph feature, so that they don't have to install the dependencies. You can achieve this by installing ppx\_const and ppx\_getenv, and invoking the graph feature like this:

	if%const [%getenv "BUILD_OMIT_GRAPH"] = "" then
		Graph.create filename
	else
		print_endline "Graph feature not available."

For this to work, you'll need to make **certain** that ppx\_getenv runs before ppx\_const, so that `if%const` sees a constant string and not the `[%` node. In ocamlbuild, you do this by ordering them like this in your `_tags` file:

	<*>: package(ppx_getenv, ppx_const)

When you build, if the `BUILD_OMIT_GRAPH` environment variable is set to a nonempty string, the Graph.create call will be omitted entirely from the compiled binary. If this is the only invocation, the Graph module and all its dependencies will also be omitted from the binary. If you do not set this environment variable, the `[%getenv` check will return an empty string at build time and the graph function will be included.

License
-------

[Creative Commons Zero](LICENSE.txt) ("public domain or equivalent")
